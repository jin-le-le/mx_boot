using System;
using System.IO;
using System.IO.Ports;
using System.Threading;
using System.Windows.Forms;
using System.Diagnostics;
using System.Security.Cryptography;

namespace MCU_BOOT_Tool
{
    public partial class Form1 : Form
    {
        // Device state machine (drives status indicator color + button enablement)
        private enum DeviceState
        {
            Disconnected,   // Gray - serial port not opened
            WaitDevice,     // Yellow - serial port open, polling
            DeviceReady,    // Green - device ready
            Uploading,      // Blue - upgrade in progress
        }

        private MCUProtocol? _protocol;
        private volatile bool _isConnected = false;
        private volatile bool _isUploading = false;
        private string? _firmwarePath;
        private FirmwareImage? _firmware;
        private byte[]? _preBuiltHeader = null;  // Pre-built header for pre-signed firmware (256B)

        // Phase 3 state
        private volatile DeviceState _deviceState = DeviceState.Disconnected;
        private DeviceInfo? _deviceInfo = null;
        private Thread? _pollingThread = null;
        private volatile bool _pollingCancel = false;

        // Default addresses (kept for backward compatibility with old firmware;
        // newer firmware reports them via device_info)
        private uint APP_ADDRESS = 0x08010000;
        private uint HEADER_ADDRESS = 0x0800C000;
        private const int POLL_INTERVAL_MS = 100;
        private const int POLL_TIMEOUT_MS = 30000;

        // Color constants
        private static readonly System.Drawing.Color ColorSuccess = System.Drawing.Color.FromArgb(16, 124, 16);
        private static readonly System.Drawing.Color ColorWarning = System.Drawing.Color.FromArgb(255, 140, 0);
        private static readonly System.Drawing.Color ColorPrimary = System.Drawing.Color.FromArgb(0, 120, 212);
        // Status indicator colors (4-state + error)
        // Gray=Disconnected, Gold=WaitDevice, Green=DeviceReady, Blue=Uploading, Red=Error
        private static readonly System.Drawing.Color ColorWait = System.Drawing.Color.FromArgb(255, 193, 7);       // Gold: waiting for device
        private static readonly System.Drawing.Color ColorUploading = System.Drawing.Color.FromArgb(0, 120, 212);  // Blue: upgrade in progress

        public Form1()
        {
            InitializeComponent();
            SetControlsEnabled(false);
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            RefreshPorts();
        }

        private void Log(string msg)
        {
            if (IsDisposed || !IsHandleCreated) return;

            var timestamp = DateTime.Now.ToString("HH:mm:ss");
            var line = $"[{timestamp}] {msg}";

            BeginInvoke(new Action(() =>
            {
                if (!IsDisposed && textBox_Log != null)
                    textBox_Log.AppendText(line + Environment.NewLine);
            }));
        }

        private void RefreshPorts()
        {
            comboBox_Port.Items.Clear();
            string[] ports = SerialPort.GetPortNames();
            Array.Sort(ports);
            comboBox_Port.Items.AddRange(ports);

            if (ports.Length > 0)
                comboBox_Port.SelectedIndex = 0;
        }

        private void SetControlsEnabled(bool enabled)
        {
            if (IsDisposed || !IsHandleCreated) return;

            BeginInvoke(new Action(() =>
            {
                if (IsDisposed) return;
                button_Upload.Enabled = enabled && !_isUploading;
                button_Reset.Enabled = enabled;
                button_Browse.Enabled = !_isUploading;
            }));
        }

        /// <summary>
        /// Unified entry point for the device state machine: switches the status indicator color,
        /// button text, control enablement, and clears labels as needed.
        /// Thread-safe: callable from either the polling thread or the UI thread.
        /// </summary>
        private void SetDeviceState(DeviceState state)
        {
            _deviceState = state;
            _isConnected = (state != DeviceState.Disconnected);
            _isUploading = (state == DeviceState.Uploading);
            if (state == DeviceState.Disconnected)
            {
                _deviceInfo = null;
            }

            if (IsDisposed || !IsHandleCreated) return;

            BeginInvoke(new Action(() =>
            {
                if (IsDisposed) return;

                switch (state)
                {
                    case DeviceState.Disconnected:
                        label_Status.ForeColor = System.Drawing.Color.Gray;
                        button_Connect.Text = "Query Device";
                        button_Connect.BackColor = ColorPrimary;
                        button_Connect.Enabled = true;
                        label_DevBLVersion.Text = "--";
                        label_DevAppVersion.Text = "--";
                        label_DevMTU.Text = "--";
                        label_DevCaps.Text = "--";
                        textBox_FwVersion.Text = "--";
                        button_Upload.Enabled = false;
                        button_Reset.Enabled = false;
                        break;

                    case DeviceState.WaitDevice:
                        label_Status.ForeColor = ColorWait;
                        button_Connect.Text = "Disconnect";
                        button_Connect.BackColor = ColorWarning;
                        button_Connect.Enabled = true;
                        button_Upload.Enabled = false;
                        button_Reset.Enabled = false;
                        break;

                    case DeviceState.DeviceReady:
                        label_Status.ForeColor = ColorSuccess;
                        button_Connect.Text = "Disconnect";
                        button_Connect.BackColor = ColorWarning;
                        button_Connect.Enabled = true;
                        button_Upload.Enabled = true;
                        button_Reset.Enabled = true;
                        button_Browse.Enabled = true;
                        textBox_FwVersion.Enabled = true;
                        break;

                    case DeviceState.Uploading:
                        label_Status.ForeColor = ColorUploading;
                        button_Connect.Enabled = false;
                        button_Upload.Enabled = false;
                        button_Reset.Enabled = false;
                        button_Browse.Enabled = false;
                        textBox_FwVersion.Enabled = false;
                        break;
                }
            }));
        }

        /// <summary>
        /// Polling thread: sends a GetDeviceInfo request every POLL_INTERVAL_MS (silent, no log spam),
        /// with an overall timeout of POLL_TIMEOUT_MS.
        /// Success -> parse device info -> switch to DeviceReady -> exit.
        /// Timeout -> show a popup -> switch to Disconnected -> exit.
        /// _pollingCancel=true -> exit immediately.
        /// Logging policy: one "started querying" line at start, one "still querying... Xs left"
        /// line every 10 s, and one summary line at the end.
        /// </summary>
        private void PollingWorker()
        {
            DateTime startTime = DateTime.Now;
            DateTime lastStatusLog = startTime;
            int pollCount = 0;
            const int STATUS_LOG_INTERVAL_MS = 10000;  // status hint every 10 seconds

            while (!_pollingCancel)
            {
                TimeSpan elapsed = DateTime.Now - startTime;

                if (elapsed.TotalMilliseconds > POLL_TIMEOUT_MS)
                {
                    // Timeout
                    int secs = POLL_TIMEOUT_MS / 1000;
                    BeginInvoke(new Action(() =>
                    {
                        Log($"[W] No device detected within {secs}s, polling stopped ({pollCount} attempts)");
                        MessageBox.Show(
                            $"No device response within {secs} seconds.\n\nPlease check:\n" +
                            "  1. Is the correct COM port selected?\n" +
                            "  2. Has the device been reset / powered on? (The bootloader only responds during the first 3 seconds.)\n" +
                            "  3. Are the wires connected correctly? (USART3: PC10/PC11)",
                            "Query Timeout", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    }));
                    BeginInvoke(new Action(() => DoDisconnect()));
                    return;
                }

                // Periodic status hint (keeps the log from going totally silent so the user knows we are still waiting)
                if ((DateTime.Now - lastStatusLog).TotalMilliseconds >= STATUS_LOG_INTERVAL_MS)
                {
                    int remaining = (int)Math.Ceiling((POLL_TIMEOUT_MS - elapsed.TotalMilliseconds) / 1000.0);
                    Log($"[I] Still querying device... {remaining}s left (please reset or power on the device)");
                    lastStatusLog = DateTime.Now;
                }

                pollCount++;
                DeviceInfo? info = null;
                try
                {
                    // verbose:false suppresses per TX/RX log lines, otherwise we'd log hundreds of lines in 30 s
                    info = _protocol?.GetDeviceInfo(POLL_INTERVAL_MS, verbose: false);
                }
                catch (Exception ex)
                {
                    Log($"[E] Polling exception: {ex.Message}");
                }

                if (_pollingCancel) return;

                if (info != null)
                {
                    // Success - update UI on UI thread
                    _deviceInfo = info;

                    // Update addresses from the device response (supported by new firmware; old firmware uses defaults)
                    if (info.AppBase != 0)
                    {
                        APP_ADDRESS = info.AppBase;
                        HEADER_ADDRESS = info.HeaderAddr;
                    }

                    BeginInvoke(new Action(() =>
                    {
                        label_DevBLVersion.Text = info.BlVersion;
                        label_DevAppVersion.Text = info.AppVersion;
                        label_DevMTU.Text = info.Mtu.ToString();
                        label_DevCaps.Text = MCUProtocol.FormatCapabilities(info.Caps);
                        SetDeviceState(DeviceState.DeviceReady);
                        Log($"[I] Device ready: BL=v{info.BlVersion}, APP={(info.HasApp ? "v" + info.AppVersion : "N/A")}, MTU={info.Mtu}");
                        Log($"[I] App address: 0x{APP_ADDRESS:X8}, Header address: 0x{HEADER_ADDRESS:X8}");
                    }));
                    return;
                }

                // No response - sleep briefly before next poll
                Thread.Sleep(POLL_INTERVAL_MS);
            }
            // Cancelled by user
        }

        /// <summary>
        /// Actually performs the disconnect: stops the polling thread, closes the serial port,
        /// and switches state to Disconnected. Must be called on the UI thread.
        /// </summary>
        private void DoDisconnect()
        {
            // Stop polling thread if running
            if (_pollingThread != null && _pollingThread.IsAlive)
            {
                _pollingCancel = true;
                try { _pollingThread.Join(500); } catch { }
                _pollingThread = null;
            }
            _pollingCancel = false;

            // Close protocol
            try { _protocol?.Close(); } catch { }
            _protocol = null;

            SetDeviceState(DeviceState.Disconnected);
            Log("[I] Disconnected");
        }

        private void UpdateProgress(int value, int max, int speed = 0)
        {
            if (IsDisposed || !IsHandleCreated) return;

            BeginInvoke(new Action(() =>
            {
                if (IsDisposed) return;
                progressBar.Maximum = max;
                progressBar.Value = Math.Min(value, max);

                int pct = max > 0 ? (int)((long)value * 100 / max) : 0;
                label_Progress.Text = $"{pct}%";

                if (speed > 0)
                {
                    if (speed > 1024 * 1024)
                    {
                        label_Speed.Text = $"{speed / 1024.0 / 1024.0:F1} MB/s";
                    }
                    else if (speed > 1024)
                    {
                        label_Speed.Text = $"{speed / 1024.0:F1} KB/s";
                    }
                    else
                    {
                        label_Speed.Text = $"{speed} B/s";
                    }
                }
            }));
        }

        private void UpdateFirmwareInfo(FirmwareImage? fw)
        {
            if (IsDisposed || !IsHandleCreated) return;

            BeginInvoke(new Action(() =>
            {
                if (IsDisposed) return;
                if (fw != null)
                {
                    label_FWSize.Text = $"{fw.Size} bytes ({fw.Size / 1024.0:F1} KB)";
                    label_FWCRC.Text = $"0x{fw.CRC32:X8}";
                    label_FWTarget.Text = $"0x{APP_ADDRESS:X8}";
                    label_FWStatus.Text = "● Loaded";
                    label_FWStatus.ForeColor = ColorSuccess;
                }
                else
                {
                    label_FWSize.Text = "--";
                    label_FWCRC.Text = "--";
                    label_FWStatus.Text = "--";
                    label_FWStatus.ForeColor = System.Drawing.Color.Gray;
                }
            }));
        }

        // ==================== Event Handlers ====================

        private void button_Refresh_Click(object sender, EventArgs e)
        {
            RefreshPorts();
            Log("[I] Ports refreshed");
        }

        private void button_Connect_Click(object sender, EventArgs e)
        {
            if (_deviceState != DeviceState.Disconnected)
            {
                // Currently connected/polling → disconnect
                DoDisconnect();
                return;
            }

            // Disconnected -> connect: open port + start polling
            string port = comboBox_Port.Text;
            if (string.IsNullOrEmpty(port))
            {
                MessageBox.Show("Please select a COM port", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            if (!this.IsHandleCreated)
            {
                MessageBox.Show("Window not ready, please retry", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            int baudrate = int.Parse(comboBox_Baudrate.Text);
            try
            {
                _protocol = new MCUProtocol(port, baudrate);
                _protocol.OnLog += Log;
                _protocol.Open();
                Log($"[I] Opened serial port {port} @ {baudrate} baud");
            }
            catch (Exception ex)
            {
                Log($"[E] Failed to open serial port: {ex.Message}");
                _protocol = null;
                label_Status.Text = "●";
                label_Status.ForeColor = System.Drawing.Color.Red;
                MessageBox.Show($"Failed to open serial port: {ex.Message}", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            // Switch to WaitDevice and launch polling thread
            _pollingCancel = false;
            SetDeviceState(DeviceState.WaitDevice);
            Log($"[I] Querying device... (please reset or power on the device within {POLL_TIMEOUT_MS / 1000}s)");

            _pollingThread = new Thread(PollingWorker)
            {
                IsBackground = true,
                Name = "PollingWorker"
            };
            _pollingThread.Start();
        }

        private void button_Browse_Click(object sender, EventArgs e)
        {
            using (var ofd = new OpenFileDialog())
            {
                ofd.Filter = "Firmware Files (*.bin)|*.bin|All Files (*.*)|*.*";
                if (ofd.ShowDialog() == DialogResult.OK)
                {
                    _firmwarePath = ofd.FileName;
                    textBox_FirmwarePath.Text = _firmwarePath;
                    _preBuiltHeader = null;  // reset

                    try
                    {
                        byte[] fileData = File.ReadAllBytes(_firmwarePath);

                        /* Check whether this is a pre-signed combined file (header + app concatenated).
                         * Criterion: the first 4 bytes == "MAGI" (0x4D414749). */
                        uint magic = fileData.Length >= 4
                            ? BitConverter.ToUInt32(fileData, 0) : 0;

                        if (magic == 0x4D414749 && fileData.Length > 256)
                        {
                            /* ====== Pre-signed combined file ======
                             * First 256 bytes = header (includes CRC + ECDSA signature + version)
                             * From byte 257 onwards = app firmware
                             * The host tool does not need the key; just upload it. */
                            _preBuiltHeader = new byte[256];
                            Array.Copy(fileData, 0, _preBuiltHeader, 0, 256);

                            byte[] appData = new byte[fileData.Length - 256];
                            Array.Copy(fileData, 256, appData, 0, appData.Length);
                            _firmware = new FirmwareImage(appData);

                            /* Extract the version from the header to show the user. */
                            uint verMaj = BitConverter.ToUInt32(fileData, 28);
                            uint verMin = BitConverter.ToUInt32(fileData, 32);
                            uint verBld = BitConverter.ToUInt32(fileData, 36);
                            textBox_FwVersion.Text = $"{verMaj}.{verMin}.{verBld}";

                            UpdateFirmwareInfo(_firmware);
                            Log($"[I] Pre-signed firmware: {Path.GetFileName(_firmwarePath)}");
                            Log($"[I]   Header: 256 bytes (includes ECDSA signature, version v{verMaj}.{verMin}.{verBld})");
                            Log($"[I]   App:    {_firmware.Size} bytes ({_firmware.Size/1024.0:F1} KB)");
                            Log($"[I]   No key required on the host side; just click Upload");
                        }
                        else
                        {
                            /* ====== Raw app.bin (legacy mode) ======
                             * The host computes the CRC and HMAC signature itself. */
                            _firmware = FirmwareImage.FromFile(_firmwarePath);
                            UpdateFirmwareInfo(_firmware);
                            Log($"[I] Loaded firmware: {Path.GetFileName(_firmwarePath)} (raw format)");
                        }
                    }
                    catch (Exception ex)
                    {
                        Log($"[E] Failed to load firmware: {ex.Message}");
                        _firmware = null;
                        _preBuiltHeader = null;
                        UpdateFirmwareInfo(null);
                    }
                }
            }
        }

        private void button_Upload_Click(object sender, EventArgs e)
        {
            if (_deviceState != DeviceState.DeviceReady)
            {
                MessageBox.Show("Device not ready; please query the device before upgrading.", "Notice", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }

            if (_firmware == null || string.IsNullOrEmpty(_firmwarePath))
            {
                MessageBox.Show("Please select a firmware file first", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            SetDeviceState(DeviceState.Uploading);
            label_Progress.Text = "Uploading...";
            progressBar.Value = 0;

            ThreadPool.QueueUserWorkItem(_ =>
            {
                try
                {
                    UploadFirmware();
                }
                catch (Exception ex)
                {
                    Log($"[E] Upgrade exception: {ex.Message}");
                }
                finally
                {
                    BeginInvoke(new Action(() =>
                    {
                        // If UploadFirmware returns normally but did not switch state
                        // (early return / failure / exception), conservatively restore
                        // DeviceReady. On a successful Boot, UploadFirmware calls
                        // DoDisconnect internally and the state becomes Disconnected.
                        if (_deviceState == DeviceState.Uploading)
                            SetDeviceState(DeviceState.DeviceReady);
                        label_Progress.Text = _deviceState == DeviceState.Disconnected ? "Done" : "Ready";
                    }));
                }
            });
        }

        /// <summary>
        /// Version comparison: firmware version (textBox_FwVersion) vs device App version (_deviceInfo).
        /// Returns true to allow the upgrade, false to cancel.
        /// </summary>
        private bool CheckFirmwareVersion()
        {
            if (_deviceInfo == null)
            {
                Log("[E] Device info not ready; cannot check version");
                return false;
            }

            // Parse firmware version from TextBox
            var parts = textBox_FwVersion.Text.Trim().Split('.');
            int fwMajor = 1, fwMinor = 0, fwBuild = 0;
            if (parts.Length >= 1) int.TryParse(parts[0], out fwMajor);
            if (parts.Length >= 2) int.TryParse(parts[1], out fwMinor);
            if (parts.Length >= 3) int.TryParse(parts[2], out fwBuild);
            int fwVer = fwMajor * 10000 + fwMinor * 100 + fwBuild;
            string fwVerStr = $"{fwMajor}.{fwMinor}.{fwBuild}";

            // No app on device -> allow any version
            if (!_deviceInfo.HasApp)
            {
                Log($"[I] Device has no App (empty Flash); allowing upgrade to any version v{fwVerStr}");
                return true;
            }

            int devVer = _deviceInfo.AppVersionInt;
            string devVerStr = _deviceInfo.AppVersion;

            if (fwVer > devVer)
            {
                Log($"[I] Version upgrade: v{devVerStr} -> v{fwVerStr}");
                return true;
            }

            if (fwVer == devVer)
            {
                var r = MessageBox.Show(
                    $"Reinstall same version?\n\n" +
                    $"Device version: v{devVerStr}\n" +
                    $"Firmware version: v{fwVerStr}\n\n" +
                    $"Click \"Yes\" to continue, \"No\" to cancel.",
                    "Same Version", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                if (r == DialogResult.Yes)
                {
                    Log($"[I] User confirmed same-version reinstall v{fwVerStr}");
                    return true;
                }
                Log("[I] User cancelled same-version reinstall");
                return false;
            }

            // Downgrade
            var rr = MessageBox.Show(
                $"⚠️ Version downgrade warning!\n\n" +
                $"Device current: v{devVerStr}\n" +
                $"Firmware version: v{fwVerStr}\n\n" +
                $"Downgrading may reintroduce bugs or security vulnerabilities that were already fixed.\n" +
                $"Are you sure you want to continue?",
                "Downgrade Warning", MessageBoxButtons.YesNo, MessageBoxIcon.Warning);
            if (rr == DialogResult.Yes)
            {
                Log($"[W] User confirmed downgrade v{devVerStr} -> v{fwVerStr}");
                return true;
            }
            Log("[I] User cancelled downgrade");
            return false;
        }

        private void UploadFirmware()
        {
            var fw = _firmware!;
            var protocol = _protocol!;

            Log($"[I] Starting upgrade: {Path.GetFileName(_firmwarePath)}");
            Log($"[I] Size: {fw.Size} bytes ({fw.Size / 1024.0:F1} KB), CRC32: 0x{fw.CRC32:X8}");

            // 1. Version check (against device's current App version)
            if (!CheckFirmwareVersion())
            {
                Log("[I] Upgrade cancelled");
                return;
            }

            // 2. Pull MTU from cached device info (avoid extra round-trip)
            uint mtu = _deviceInfo?.Mtu ?? 4096;
            Log($"[I] MTU: {mtu} bytes ({mtu - 8} bytes of firmware data per packet)");

            // Parse version for header later
            var versionParts = textBox_FwVersion.Text.Split('.');
            int major = 1, minor = 0, build = 0;
            if (versionParts.Length >= 1) int.TryParse(versionParts[0], out major);
            if (versionParts.Length >= 2) int.TryParse(versionParts[1], out minor);
            if (versionParts.Length >= 3) int.TryParse(versionParts[2], out build);

            // Erase
            BeginInvoke(new Action(() => label_Progress.Text = "Erasing..."));
            Log($"[I] Erasing 0x{APP_ADDRESS:X8} ({fw.Size} bytes)...");
            if (!protocol.Erase(APP_ADDRESS, fw.Size))
            {
                Log("[E] Erase failed");
                return;
            }
            Log("[OK] Erase completed");

            // Program
            BeginInvoke(new Action(() => label_Progress.Text = "Programming..."));
            int chunkSize = (int)mtu - 8;
            int totalSize = (int)fw.Size;
            int bytesSent = 0;
            var stopwatch = Stopwatch.StartNew();

            while (bytesSent < totalSize)
            {
                int remaining = totalSize - bytesSent;
                int thisChunk = Math.Min(chunkSize, remaining);
                var chunk = new byte[thisChunk];
                Array.Copy(fw.Data, bytesSent, chunk, 0, thisChunk);

                uint chunkCrc = FirmwareCRC32.Compute(chunk);
                Log($"[D] Program chunk: offset={bytesSent}, size={thisChunk}, crc=0x{chunkCrc:X8}");

                if (!protocol.Program(APP_ADDRESS + (uint)bytesSent, chunk))
                {
                    Log("[E] Program failed");
                    return;
                }

                bytesSent += thisChunk;

                stopwatch.Stop();
                int speed = (int)((long)bytesSent * 1000 / Math.Max(1, stopwatch.ElapsedMilliseconds));
                BeginInvoke(new Action(() => UpdateProgress(bytesSent, totalSize, speed)));
                stopwatch.Start();
            }
            Log("[OK] Programming completed");

            // Verify
            BeginInvoke(new Action(() => label_Progress.Text = "Verifying..."));
            Log($"[I] Verifying CRC32=0x{fw.CRC32:X8} at 0x{APP_ADDRESS:X8} size={fw.Size}...");
            Log($"[D] Host: fw.Data.Length={fw.Data.Length}, first_byte=0x{fw.Data[0]:X2}, last_byte_offset={fw.Size - 1}, last_byte=0x{fw.Data[fw.Data.Length - 1]:X2}");
            Log($"[D] Host full CRC: 0x{fw.CRC32:X8}");
            if (!protocol.Verify(APP_ADDRESS, fw.Size, fw.CRC32))
            {
                Log("[E] Verify failed - CRC mismatch");
                return;
            }
            Log("[OK] Verification passed");
            BeginInvoke(new Action(() => UpdateProgress(totalSize, totalSize)));

            // Write header
            BeginInvoke(new Action(() => label_Progress.Text = "Writing header..."));

            byte[] header;
            if (_preBuiltHeader != null)
            {
                /* ====== Pre-signed firmware: header is already in the file, use it directly ======
                 * No key required, no signature computation needed - the CRC, signature and
                 * version inside the header are all preset by the Python script. */
                Log("[I] Using pre-built header from file (includes ECDSA signature)...");
                header = _preBuiltHeader;
            }
            else
            {
                /* ====== Raw app.bin: build an unsigned header ====== */
                Log("[W] Raw firmware (unsigned); building unsigned header...");
                Log("[W] Consider using the Python signing tool to produce a signed firmware (./sign.sh app.bin <version>)");
                header = fw.BuildHeader(major, minor, build, 0, null);
            }

            if (!protocol.WriteHeader(header))
            {
                Log("[E] Header write failed");
                return;
            }
            Log($"[OK] Header written to 0x{HEADER_ADDRESS:X8}");

            // Boot
            BeginInvoke(new Action(() => label_Progress.Text = "Booting..."));
            Log("[I] Sending BOOT command...");
            if (!protocol.Boot())
            {
                Log("[E] Boot command failed");
                return;
            }
            Log("[OK] Boot command sent!");

            BeginInvoke(new Action(() =>
            {
                label_Progress.Text = "Upload Complete!";
                UpdateProgress(totalSize, totalSize);
            }));

            // Print upgrade statistics
            stopwatch.Stop();
            long totalMs = stopwatch.ElapsedMilliseconds;
            double totalSec = totalMs / 1000.0;
            int finalSpeed = (int)((long)totalSize * 1000 / Math.Max(1, totalMs));

            Log("");
            Log("========== Upgrade Statistics ==========");
            Log($"Total time: {totalSec:F2} s ({totalMs} ms)");
            Log($"Firmware size: {totalSize} bytes ({totalSize / 1024.0:F1} KB)");
            Log($"Average speed: {finalSpeed / 1024.0:F1} KB/s");

            // Baud rate analysis
            int currentBaud = _protocol.GetCurrentBaudrate();
            double currentKBps = currentBaud / 10.0 / 1024.0;
            double actualKBps = finalSpeed / 1024.0;
            double efficiency = actualKBps / currentKBps * 100;

            Log("");
            Log("========== Baud Rate Analysis ==========");
            Log($"Current baud rate: {currentBaud} bps");
            Log($"Theoretical speed: {currentKBps:F1} KB/s");
            Log($"Actual speed: {actualKBps:F1} KB/s");
            Log($"Transfer efficiency: {efficiency:F1}%");
            Log("==============================");
            Log("[OK] Upgrade complete! The device is about to leave the bootloader; disconnecting");
            BeginInvoke(new Action(() => DoDisconnect()));
        }

        private void button_Reset_Click(object sender, EventArgs e)
        {
            if (_deviceState != DeviceState.DeviceReady)
            {
                MessageBox.Show("Device not ready; please query the device first.", "Notice", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }

            if (MessageBox.Show("Confirm device reset?", "Confirm", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
            {
                Log("[I] Sending reset command...");
                if (_protocol!.Reset())
                {
                    Log("[OK] Reset command sent");
                    // After reset the device leaves the bootloader; disconnect and wait for the user to re-query.
                    DoDisconnect();
                }
            }
        }

        private void button_ClearLog_Click(object sender, EventArgs e)
        {
            textBox_Log.Clear();
        }
    }
}
