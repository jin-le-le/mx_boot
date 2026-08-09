using System;
using System.IO.Ports;
using System.Threading;

namespace MCU_BOOT_Tool
{
    /// <summary>
    /// Device info (matches the device-side device_info_t).
    /// Layout: BL version (3) + has_app (1) + App version (3) + MTU (2) + Caps (4) +
    ///         header_addr (4) + app_base (4) + reserved (3) = 24B
    /// </summary>
    internal class DeviceInfo
    {
        public byte BlMajor, BlMinor, BlBuild;
        public bool HasApp;
        public byte AppMajor, AppMinor, AppBuild;
        public ushort Mtu;
        public MCUProtocol.Capabilities Caps;
        public uint HeaderAddr;   // Device-side header write address (BL_MAGIC_HEADER_ADDR)
        public uint AppBase;      // Device-side App base address (BL_APP_BASE)

        public string BlVersion  => $"{BlMajor}.{BlMinor}.{BlBuild}";
        public string AppVersion => HasApp ? $"{AppMajor}.{AppMinor}.{AppBuild}" : "N/A";

        public int AppVersionInt => AppMajor * 10000 + AppMinor * 100 + AppBuild;

        public static DeviceInfo Parse(byte[] data)
        {
            if (data == null || data.Length < 17) return null;
            var info = new DeviceInfo
            {
                BlMajor  = data[0],
                BlMinor  = data[1],
                BlBuild  = data[2],
                HasApp   = data[3] != 0,
                AppMajor = data[4],
                AppMinor = data[5],
                AppBuild = data[6],
                Mtu      = (ushort)(data[7] | (data[8] << 8)),
                Caps     = (MCUProtocol.Capabilities)(uint)(
                    data[9] | (data[10] << 8) | (data[11] << 16) | (data[12] << 24))
            };
            // header_addr and app_base are newer fields; older firmware may not include them (data.Length < 21).
            if (data.Length >= 21)
            {
                info.HeaderAddr = (uint)(data[13] | (data[14] << 8) | (data[15] << 16) | (data[16] << 24));
                info.AppBase    = (uint)(data[17] | (data[18] << 8) | (data[19] << 16) | (data[20] << 24));
            }
            return info;
        }
    }

    /// <summary>
    /// MCU_BOOT communication protocol.
    /// Protocol format:
    ///   Request:  [0xAA] [Opcode] [Length_L] [Length_H] [Payload...] [CRC16_L] [CRC16_H]
    ///   Response: [0x55] [Opcode] [ErrCode]  [Length_L] [Length_H] [Payload...] [CRC16_L] [CRC16_H]
    /// </summary>
    internal class MCUProtocol
    {
        // Opcodes
        public enum Opcode : byte
        {
            INQUERY = 0x01,
            GET_CAPS = 0x04,
            RESET = 0x21,
            BOOT = 0x22,
            ERASE = 0x81,
            PROGRAM = 0x82,
            VERIFY = 0x83,
            WRITE_HEADER = 0x84,
        }

        // INQUERY sub-commands
        public enum InquirySub : byte
        {
            DEVICE_INFO = 0x00,   // 16B binary device_info_t (BL+App version, MTU, Caps)
        }

        // Error codes
        public enum ErrorCode : byte
        {
            OK = 0x00,
            OPCODE = 0x01,       // Invalid opcode
            OVERFLOW = 0x02,     // Buffer overflow
            TIMEOUT = 0x03,      // Receive timeout
            FORMAT = 0x04,       // Format error
            VERIFY = 0x05,       // Verification failed
            PARAM = 0x06,        // Invalid parameter
            SIGNATURE = 0x07,    // Signature verification failed
            UNKNOWN = 0xFF,      // Unknown error
        }

        // Capability flags
        [Flags]
        public enum Capabilities : uint
        {
            NONE = 0,
            SIGNED = 0x01,
            ANTIROLLBACK = 0x02,
            CRC = 0x04,
            DUAL_BANK = 0x08,
        }

        /// <summary>
        /// Format capability flags as a human-readable string
        /// </summary>
        public static string FormatCapabilities(Capabilities caps)
        {
            if (caps == Capabilities.NONE)
                return "NONE";
            var parts = new System.Collections.Generic.List<string>();
            if (caps.HasFlag(Capabilities.SIGNED)) parts.Add("SIGNED");
            if (caps.HasFlag(Capabilities.ANTIROLLBACK)) parts.Add("ANTIROLLBACK");
            if (caps.HasFlag(Capabilities.CRC)) parts.Add("CRC");
            if (caps.HasFlag(Capabilities.DUAL_BANK)) parts.Add("DUAL_BANK");
            // Check for any undefined bits
            uint unknownBits = (uint)caps & ~0x0Fu;
            if (unknownBits != 0) parts.Add($"0x{unknownBits:X}");
            return string.Join(" | ", parts);
        }

        // Delegate
        public delegate void LogHandler(string msg);
        public event LogHandler? OnLog;

        // Constants
        private const byte HEADER_REQUEST = 0xAA;
        private const byte HEADER_RESPONSE = 0x55;
        private const int DEFAULT_TIMEOUT = 3000;

        // Serial port
        private readonly SerialPort _serialPort;
        private readonly AutoResetEvent _receiveEvent = new AutoResetEvent(false);

        public bool IsOpen => _serialPort.IsOpen;

        public MCUProtocol(string port, int baudrate = 115200)
        {
            _serialPort = new SerialPort(port, baudrate)
            {
                ReadTimeout = 1000,
                WriteTimeout = 1000,
                WriteBufferSize = 4096,
                ReadBufferSize = 4096,
            };
            _serialPort.DataReceived += DataReceivedHandler;
        }

        public void Open()
        {
            if (!_serialPort.IsOpen)
                _serialPort.Open();
        }

        public void Close()
        {
            if (_serialPort.IsOpen)
            {
                _serialPort.Close();
                _serialPort.Dispose();
            }
        }

        public void SetPort(string port)
        {
            if (_serialPort.IsOpen)
            {
                _serialPort.Close();
            }
            _serialPort.PortName = port;
            _serialPort.Open();
        }

        /// <summary>
        /// Get the current baud rate
        /// </summary>
        public int GetCurrentBaudrate()
        {
            return _serialPort.BaudRate;
        }

        private void Log(string msg)
        {
            OnLog?.Invoke(msg);
        }

        /// <summary>
        /// Build a request packet
        /// </summary>
        private byte[] BuildPacket(Opcode op, byte[]? payload = null)
        {
            ushort length = payload != null ? (ushort)payload.Length : (ushort)0;

            // Build packet (without CRC)
            var packet = new byte[4 + length];
            packet[0] = HEADER_REQUEST;
            packet[1] = (byte)op;
            packet[2] = (byte)(length & 0xFF);
            packet[3] = (byte)((length >> 8) & 0xFF);

            if (payload != null)
                Array.Copy(payload, 0, packet, 4, length);

            // Compute CRC16
            ushort crc = CRC16Modbus.Compute(packet, 0, packet.Length);

            // Full packet = header + payload + crc
            var fullPacket = new byte[packet.Length + 2];
            Array.Copy(packet, 0, fullPacket, 0, packet.Length);
            fullPacket[fullPacket.Length - 2] = (byte)(crc & 0xFF);
            fullPacket[fullPacket.Length - 1] = (byte)((crc >> 8) & 0xFF);

            return fullPacket;
        }

        /// <summary>
        /// Parse a response packet
        /// </summary>
        private bool ParseResponse(byte[] data, out Opcode opcode, out ErrorCode errCode, out byte[]? payload)
        {
            opcode = Opcode.INQUERY;
            errCode = ErrorCode.UNKNOWN;
            payload = null;

            if (data.Length < 1 || data[0] != HEADER_RESPONSE)
            {
                Log($"Header error: 0x{data[0]:X2}");
                return false;
            }

            if (data.Length < 4)
            {
                Log("Response too short");
                return false;
            }

            opcode = (Opcode)data[1];
            errCode = (ErrorCode)data[2];
            // Response format: [0x55][op][err][lenL][lenH][payload...][crcL][crcH]
            ushort length = (ushort)(data[3] | (data[4] << 8));

            int expectedLen = 1 + 1 + 1 + 2 + length + 2; // header + opcode + err + len + payload + crc
            if (data.Length != expectedLen)
            {
                Log($"Length error: got {data.Length}, expected {expectedLen}");
                return false;
            }

            // Verify CRC
            ushort crcReceived = (ushort)(data[data.Length - 2] | (data[data.Length - 1] << 8));
            ushort crcComputed = CRC16Modbus.Compute(data, 0, data.Length - 2);
            if (crcReceived != crcComputed)
            {
                Log($"CRC error: got 0x{crcReceived:X4}, computed 0x{crcComputed:X4}");
                return false;
            }

            // Extract payload
            if (length > 0)
            {
                payload = new byte[length];
                Array.Copy(data, 5, payload, 0, length);
            }

            return true;
        }

        /// <summary>
        /// Send a request and wait for the response
        /// </summary>
        /// <param name="verbose">true = print TX/RX debug logs (default); false = silent (used during polling to avoid log spam)</param>
        private bool SendRequest(Opcode opcode, byte[]? payload, out ErrorCode errCode, out byte[]? responseData, int timeout = DEFAULT_TIMEOUT, bool verbose = true)
        {
            errCode = ErrorCode.UNKNOWN;
            responseData = null;

            // Flush the receive buffer
            if (_serialPort.BytesToRead > 0)
            {
                byte[] trash = new byte[_serialPort.BytesToRead];
                _serialPort.Read(trash, 0, trash.Length);
            }

            // Send
            byte[] packet = BuildPacket(opcode, payload);
            if (verbose) Log($"TX: {BitConverter.ToString(packet)}");
            _receiveEvent.Reset();
            _serialPort.Write(packet, 0, packet.Length);

            // Wait for response
            if (!_receiveEvent.WaitOne(timeout))
            {
                if (verbose) Log("Timeout waiting for response");
                return false;
            }

            // Wait for the data to be fully received
            Thread.Sleep(50);

            int ackLen = _serialPort.BytesToRead;
            if (ackLen < 5)
            {
                if (verbose) Log($"Response too short: {ackLen} bytes");
                return false;
            }

            byte[] ack = new byte[ackLen];
            _serialPort.Read(ack, 0, ackLen);
            if (verbose) Log($"RX: {BitConverter.ToString(ack)}");

            // Parse
            Opcode respOpcode;
            if (!ParseResponse(ack, out respOpcode, out errCode, out responseData))
            {
                return false;
            }

            if (respOpcode != opcode)
            {
                if (verbose) Log($"Opcode mismatch: sent 0x{(byte)opcode:X2}, got 0x{(byte)respOpcode:X2}");
                return false;
            }

            return true;
        }

        // ==================== Public API ====================

        /// <summary>
        /// Query device info (returns BL version + App version + MTU + Caps in one shot, 16B total)
        /// </summary>
        /// <param name="timeoutMs">Per-request timeout. Set short during polling, e.g. 400 ms.</param>
        /// <param name="verbose">true = print TX/RX logs; false = silent (used during polling)</param>
        /// <returns>DeviceInfo on success, null on failure</returns>
        public DeviceInfo GetDeviceInfo(int timeoutMs = 1000, bool verbose = true)
        {
            if (!SendRequest(Opcode.INQUERY, new byte[] { (byte)InquirySub.DEVICE_INFO }, out var err, out var data, timeoutMs, verbose)
                || err != ErrorCode.OK)
            {
                return null;
            }

            return DeviceInfo.Parse(data);
        }

        /// <summary>
        /// Erase Flash
        /// </summary>
        public bool Erase(uint address, uint size)
        {
            byte[] payload = new byte[8];
            BitConverter.GetBytes(address).CopyTo(payload, 0);
            BitConverter.GetBytes(size).CopyTo(payload, 4);

            if (!SendRequest(Opcode.ERASE, payload, out var err, out _) || err != ErrorCode.OK)
            {
                Log($"Erase failed: {err}");
                return false;
            }
            return true;
        }

        /// <summary>
        /// Program Flash
        /// </summary>
        public bool Program(uint address, byte[] data)
        {
            byte[] payload = new byte[8 + data.Length];
            BitConverter.GetBytes(address).CopyTo(payload, 0);
            BitConverter.GetBytes((uint)data.Length).CopyTo(payload, 4);
            data.CopyTo(payload, 8);

            if (!SendRequest(Opcode.PROGRAM, payload, out var err, out _) || err != ErrorCode.OK)
            {
                Log($"Program failed: {err}");
                return false;
            }
            return true;
        }

        /// <summary>
        /// Verify Flash (CRC32)
        /// </summary>
        public bool Verify(uint address, uint size, uint expectedCrc)
        {
            Log($"Verify: addr=0x{address:X8}, size={size}, expectedCrc=0x{expectedCrc:X8}");
            byte[] payload = new byte[12];
            BitConverter.GetBytes(address).CopyTo(payload, 0);
            BitConverter.GetBytes(size).CopyTo(payload, 4);
            BitConverter.GetBytes(expectedCrc).CopyTo(payload, 8);

            if (!SendRequest(Opcode.VERIFY, payload, out var err, out _) || err != ErrorCode.OK)
            {
                Log($"Verify failed: {err}");
                return false;
            }
            return true;
        }

        /// <summary>
        /// Reset the device
        /// </summary>
        public bool Reset()
        {
            if (!SendRequest(Opcode.RESET, null, out var err, out _) || err != ErrorCode.OK)
            {
                Log($"Reset failed: {err}");
                return false;
            }
            return true;
        }

        /// <summary>
        /// Send the BOOT command
        /// </summary>
        public bool Boot()
        {
            if (!SendRequest(Opcode.BOOT, null, out var err, out _) || err != ErrorCode.OK)
            {
                Log($"Boot failed: {err}");
                return false;
            }
            return true;
        }

        /// <summary>
        /// Write the image header (dedicated command that bypasses the header-area safety check).
        /// The MCU handler receives the 256-byte header data directly.
        /// </summary>
        public bool WriteHeader(byte[] headerData)
        {
            if (!SendRequest(Opcode.WRITE_HEADER, headerData, out var err, out _) || err != ErrorCode.OK)
            {
                Log($"WriteHeader failed: {err}");
                return false;
            }
            return true;
        }

        private void DataReceivedHandler(object sender, SerialDataReceivedEventArgs e)
        {
            _receiveEvent.Set();
        }
    }
}
