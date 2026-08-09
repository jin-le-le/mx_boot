namespace MCU_BOOT_Tool
{
    partial class Form1
    {
        private System.ComponentModel.IContainer components = null;

        // Connection area
        private System.Windows.Forms.ComboBox comboBox_Port;
        private System.Windows.Forms.ComboBox comboBox_Baudrate;
        private System.Windows.Forms.Button button_Refresh;
        private System.Windows.Forms.Button button_Connect;
        private System.Windows.Forms.Label label_Status;

        // Device Info area
        private System.Windows.Forms.Label label_DevBLVersion;
        private System.Windows.Forms.Label label_DevAppVersion;
        private System.Windows.Forms.Label label_DevMTU;
        private System.Windows.Forms.Label label_DevCaps;

        // Firmware area
        private System.Windows.Forms.TextBox textBox_FirmwarePath;
        private System.Windows.Forms.Button button_Browse;
        private System.Windows.Forms.TextBox textBox_FwVersion;

        // Firmware Info area
        private System.Windows.Forms.Label label_FWSize;
        private System.Windows.Forms.Label label_FWCRC;
        private System.Windows.Forms.Label label_FWTarget;
        private System.Windows.Forms.Label label_FWStatus;

        // Progress area
        private System.Windows.Forms.ProgressBar progressBar;
        private System.Windows.Forms.Label label_Progress;
        private System.Windows.Forms.Label label_Speed;

        // Action buttons
        private System.Windows.Forms.Button button_Upload;
        private System.Windows.Forms.Button button_Reset;
        private System.Windows.Forms.Button button_ClearLog;

        // Log area
        private System.Windows.Forms.TextBox textBox_Log;

        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
                components.Dispose();
            base.Dispose(disposing);
        }

        private void InitializeComponent()
        {
            this.comboBox_Port = new System.Windows.Forms.ComboBox();
            this.comboBox_Baudrate = new System.Windows.Forms.ComboBox();
            this.button_Refresh = new System.Windows.Forms.Button();
            this.button_Connect = new System.Windows.Forms.Button();
            this.label_Status = new System.Windows.Forms.Label();

            this.label_DevBLVersion = new System.Windows.Forms.Label();
            this.label_DevAppVersion = new System.Windows.Forms.Label();
            this.label_DevMTU = new System.Windows.Forms.Label();
            this.label_DevCaps = new System.Windows.Forms.Label();

            this.textBox_FirmwarePath = new System.Windows.Forms.TextBox();
            this.button_Browse = new System.Windows.Forms.Button();
            this.textBox_FwVersion = new System.Windows.Forms.TextBox();

            this.label_FWSize = new System.Windows.Forms.Label();
            this.label_FWCRC = new System.Windows.Forms.Label();
            this.label_FWTarget = new System.Windows.Forms.Label();
            this.label_FWStatus = new System.Windows.Forms.Label();

            this.progressBar = new System.Windows.Forms.ProgressBar();
            this.label_Progress = new System.Windows.Forms.Label();
            this.label_Speed = new System.Windows.Forms.Label();

            this.button_Upload = new System.Windows.Forms.Button();
            this.button_Reset = new System.Windows.Forms.Button();
            this.button_ClearLog = new System.Windows.Forms.Button();

            this.textBox_Log = new System.Windows.Forms.TextBox();

            this.SuspendLayout();

            // ==================== Connection Section ====================
            // Connection label
            var label_Connection = new System.Windows.Forms.Label();
            label_Connection.Text = "Connection";
            label_Connection.Font = new System.Drawing.Font("Microsoft YaHei UI", 13F, System.Drawing.FontStyle.Bold);
            label_Connection.ForeColor = System.Drawing.Color.FromArgb(50, 50, 50);
            label_Connection.Location = new System.Drawing.Point(27, 22);
            label_Connection.AutoSize = true;
            this.Controls.Add(label_Connection);

            // Status indicator
            this.label_Status.Text = "●";
            this.label_Status.Font = new System.Drawing.Font("Segoe UI Emoji", 12F);
            this.label_Status.ForeColor = System.Drawing.Color.Gray;
            this.label_Status.Location = new System.Drawing.Point(452, 20);
            this.label_Status.AutoSize = true;
            this.Controls.Add(this.label_Status);

            // COM Port label
            var label_Port = new System.Windows.Forms.Label();
            label_Port.Text = "COM Port";
            label_Port.Font = new System.Drawing.Font("Microsoft YaHei UI", 10F);
            label_Port.ForeColor = System.Drawing.Color.FromArgb(170, 170, 170);
            label_Port.Location = new System.Drawing.Point(27, 50);
            label_Port.AutoSize = true;
            this.Controls.Add(label_Port);

            this.comboBox_Port.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBox_Port.Font = new System.Drawing.Font("Microsoft YaHei UI", 10F);
            this.comboBox_Port.Location = new System.Drawing.Point(102, 47);
            this.comboBox_Port.Size = new System.Drawing.Size(120, 26);
            this.comboBox_Port.TabIndex = 0;
            this.Controls.Add(this.comboBox_Port);

            this.button_Refresh.Text = "⟳";
            this.button_Refresh.Font = new System.Drawing.Font("Segoe UI Emoji", 12F);
            this.button_Refresh.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.button_Refresh.ForeColor = System.Drawing.Color.FromArgb(50, 50, 50);
            this.button_Refresh.BackColor = System.Drawing.Color.White;
            this.button_Refresh.Location = new System.Drawing.Point(227, 44);
            this.button_Refresh.Size = new System.Drawing.Size(35, 30);
            this.button_Refresh.TabIndex = 1;
            this.button_Refresh.UseVisualStyleBackColor = false;
            this.button_Refresh.Click += new System.EventHandler(this.button_Refresh_Click);
            this.Controls.Add(this.button_Refresh);

            // Baudrate label
            var label_Baudrate = new System.Windows.Forms.Label();
            label_Baudrate.Text = "Baudrate";
            label_Baudrate.Font = new System.Drawing.Font("Microsoft YaHei UI", 10F);
            label_Baudrate.ForeColor = System.Drawing.Color.FromArgb(170, 170, 170);
            label_Baudrate.Location = new System.Drawing.Point(272, 50);
            label_Baudrate.AutoSize = true;
            this.Controls.Add(label_Baudrate);

            this.comboBox_Baudrate.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBox_Baudrate.Font = new System.Drawing.Font("Microsoft YaHei UI", 10F);
            this.comboBox_Baudrate.Items.AddRange(new object[] { "115200", "230400", "460800", "921600" });
            this.comboBox_Baudrate.Location = new System.Drawing.Point(342, 47);
            this.comboBox_Baudrate.Size = new System.Drawing.Size(80, 26);
            this.comboBox_Baudrate.TabIndex = 2;
            this.comboBox_Baudrate.SelectedIndex = 0;
            this.Controls.Add(this.comboBox_Baudrate);

            this.button_Connect.BackColor = System.Drawing.Color.FromArgb(0, 120, 212);
            this.button_Connect.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.button_Connect.Font = new System.Drawing.Font("Microsoft YaHei UI", 10F, System.Drawing.FontStyle.Bold);
            this.button_Connect.ForeColor = System.Drawing.Color.White;
            this.button_Connect.Location = new System.Drawing.Point(420, 44);
            this.button_Connect.Name = "button_Connect";
            this.button_Connect.Size = new System.Drawing.Size(90, 32);
            this.button_Connect.TabIndex = 3;
            this.button_Connect.Text = "Query Device";
            this.button_Connect.UseVisualStyleBackColor = false;
            this.button_Connect.Click += new System.EventHandler(this.button_Connect_Click);
            this.Controls.Add(this.button_Connect);

            // ==================== Device Info Section ====================
            var label_DeviceInfoTitle = new System.Windows.Forms.Label();
            label_DeviceInfoTitle.Text = "Device Info";
            label_DeviceInfoTitle.Font = new System.Drawing.Font("Microsoft YaHei UI", 13F, System.Drawing.FontStyle.Bold);
            label_DeviceInfoTitle.ForeColor = System.Drawing.Color.FromArgb(50, 50, 50);
            label_DeviceInfoTitle.Location = new System.Drawing.Point(27, 92);
            label_DeviceInfoTitle.AutoSize = true;
            this.Controls.Add(label_DeviceInfoTitle);

            // ===== Row 1: BL Version + App Version (y=120) =====
            var label_BLTitle = new System.Windows.Forms.Label();
            label_BLTitle.Text = "BL:";
            label_BLTitle.Font = new System.Drawing.Font("Microsoft YaHei UI", 10F);
            label_BLTitle.ForeColor = System.Drawing.Color.FromArgb(170, 170, 170);
            label_BLTitle.Location = new System.Drawing.Point(27, 120);
            label_BLTitle.AutoSize = true;
            this.Controls.Add(label_BLTitle);

            this.label_DevBLVersion.Font = new System.Drawing.Font("Consolas", 11F, System.Drawing.FontStyle.Bold);
            this.label_DevBLVersion.ForeColor = System.Drawing.Color.FromArgb(16, 124, 16);
            this.label_DevBLVersion.Location = new System.Drawing.Point(55, 120);
            this.label_DevBLVersion.AutoSize = true;
            this.label_DevBLVersion.TabIndex = 10;
            this.label_DevBLVersion.Text = "--";
            this.Controls.Add(this.label_DevBLVersion);

            var label_AppTitle = new System.Windows.Forms.Label();
            label_AppTitle.Text = "APP:";
            label_AppTitle.Font = new System.Drawing.Font("Microsoft YaHei UI", 10F);
            label_AppTitle.ForeColor = System.Drawing.Color.FromArgb(170, 170, 170);
            label_AppTitle.Location = new System.Drawing.Point(170, 120);
            label_AppTitle.AutoSize = true;
            this.Controls.Add(label_AppTitle);

            this.label_DevAppVersion.Font = new System.Drawing.Font("Consolas", 11F, System.Drawing.FontStyle.Bold);
            this.label_DevAppVersion.ForeColor = System.Drawing.Color.FromArgb(16, 124, 16);
            this.label_DevAppVersion.Location = new System.Drawing.Point(210, 120);
            this.label_DevAppVersion.AutoSize = true;
            this.label_DevAppVersion.TabIndex = 11;
            this.label_DevAppVersion.Text = "--";
            this.Controls.Add(this.label_DevAppVersion);

            // ===== Row 2: MTU + Caps (y=148) =====
            var label_MTUTitle = new System.Windows.Forms.Label();
            label_MTUTitle.Text = "MTU:";
            label_MTUTitle.Font = new System.Drawing.Font("Microsoft YaHei UI", 10F);
            label_MTUTitle.ForeColor = System.Drawing.Color.FromArgb(170, 170, 170);
            label_MTUTitle.Location = new System.Drawing.Point(27, 148);
            label_MTUTitle.AutoSize = true;
            this.Controls.Add(label_MTUTitle);

            this.label_DevMTU.Font = new System.Drawing.Font("Consolas", 11F, System.Drawing.FontStyle.Bold);
            this.label_DevMTU.ForeColor = System.Drawing.Color.FromArgb(16, 124, 16);
            this.label_DevMTU.Location = new System.Drawing.Point(75, 148);
            this.label_DevMTU.AutoSize = true;
            this.label_DevMTU.TabIndex = 12;
            this.label_DevMTU.Text = "--";
            this.Controls.Add(this.label_DevMTU);

            var label_CapsTitle = new System.Windows.Forms.Label();
            label_CapsTitle.Text = "Caps:";
            label_CapsTitle.Font = new System.Drawing.Font("Microsoft YaHei UI", 10F);
            label_CapsTitle.ForeColor = System.Drawing.Color.FromArgb(170, 170, 170);
            label_CapsTitle.Location = new System.Drawing.Point(170, 148);
            label_CapsTitle.AutoSize = true;
            this.Controls.Add(label_CapsTitle);

            this.label_DevCaps.Font = new System.Drawing.Font("Consolas", 11F, System.Drawing.FontStyle.Bold);
            this.label_DevCaps.ForeColor = System.Drawing.Color.FromArgb(16, 124, 16);
            this.label_DevCaps.Location = new System.Drawing.Point(210, 148);
            this.label_DevCaps.AutoSize = true;
            this.label_DevCaps.TabIndex = 13;
            this.label_DevCaps.Text = "--";
            this.Controls.Add(this.label_DevCaps);

            // ==================== Firmware Section ====================
            var label_FirmwareTitle = new System.Windows.Forms.Label();
            label_FirmwareTitle.Text = "Firmware";
            label_FirmwareTitle.Font = new System.Drawing.Font("Microsoft YaHei UI", 13F, System.Drawing.FontStyle.Bold);
            label_FirmwareTitle.ForeColor = System.Drawing.Color.FromArgb(50, 50, 50);
            label_FirmwareTitle.Location = new System.Drawing.Point(27, 190);
            label_FirmwareTitle.AutoSize = true;
            this.Controls.Add(label_FirmwareTitle);

            // File row
            var label_FileTitle = new System.Windows.Forms.Label();
            label_FileTitle.Text = "File:";
            label_FileTitle.Font = new System.Drawing.Font("Microsoft YaHei UI", 10F);
            label_FileTitle.ForeColor = System.Drawing.Color.FromArgb(170, 170, 170);
            label_FileTitle.Location = new System.Drawing.Point(27, 218);
            label_FileTitle.AutoSize = true;
            this.Controls.Add(label_FileTitle);

            this.textBox_FirmwarePath.BackColor = System.Drawing.Color.White;
            this.textBox_FirmwarePath.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.textBox_FirmwarePath.Font = new System.Drawing.Font("Consolas", 9F);
            this.textBox_FirmwarePath.ForeColor = System.Drawing.Color.FromArgb(170, 170, 170);
            this.textBox_FirmwarePath.Location = new System.Drawing.Point(67, 216);
            this.textBox_FirmwarePath.Name = "textBox_FirmwarePath";
            this.textBox_FirmwarePath.ReadOnly = true;
            this.textBox_FirmwarePath.Size = new System.Drawing.Size(320, 23);
            this.textBox_FirmwarePath.TabIndex = 14;
            this.textBox_FirmwarePath.Text = "No file selected";
            this.Controls.Add(this.textBox_FirmwarePath);

            this.button_Browse.BackColor = System.Drawing.Color.White;
            this.button_Browse.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.button_Browse.Font = new System.Drawing.Font("Microsoft YaHei UI", 9F);
            this.button_Browse.ForeColor = System.Drawing.Color.FromArgb(50, 50, 50);
            this.button_Browse.Location = new System.Drawing.Point(392, 215);
            this.button_Browse.Name = "button_Browse";
            this.button_Browse.Size = new System.Drawing.Size(70, 26);
            this.button_Browse.TabIndex = 15;
            this.button_Browse.Text = "Browse";
            this.button_Browse.UseVisualStyleBackColor = false;
            this.button_Browse.Click += new System.EventHandler(this.button_Browse_Click);
            this.Controls.Add(this.button_Browse);

            // Version row
            var label_VerFwTitle = new System.Windows.Forms.Label();
            label_VerFwTitle.Text = "Version:";
            label_VerFwTitle.Font = new System.Drawing.Font("Microsoft YaHei UI", 10F);
            label_VerFwTitle.ForeColor = System.Drawing.Color.FromArgb(170, 170, 170);
            label_VerFwTitle.Location = new System.Drawing.Point(27, 250);
            label_VerFwTitle.AutoSize = true;
            this.Controls.Add(label_VerFwTitle);

            this.textBox_FwVersion.BackColor = System.Drawing.Color.FromArgb(240, 240, 240);
            this.textBox_FwVersion.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.textBox_FwVersion.Font = new System.Drawing.Font("Consolas", 11F, System.Drawing.FontStyle.Bold);
            this.textBox_FwVersion.ForeColor = System.Drawing.Color.FromArgb(16, 124, 16);
            this.textBox_FwVersion.Location = new System.Drawing.Point(92, 248);
            this.textBox_FwVersion.Name = "textBox_FwVersion";
            this.textBox_FwVersion.ReadOnly = true;
            this.textBox_FwVersion.Size = new System.Drawing.Size(120, 20);
            this.textBox_FwVersion.TabIndex = 16;
            this.textBox_FwVersion.Text = "N/A";
            this.Controls.Add(this.textBox_FwVersion);

            // Sign checkbox removed - HMAC signing is always on by default.
            // If you need unsigned firmware, edit flags in code instead.

            // HMAC Key (signing is enabled by default; the key is always required)
            // ==================== Firmware Info Section ====================
            var label_FWInfoTitle = new System.Windows.Forms.Label();
            label_FWInfoTitle.Text = "Firmware Info";
            label_FWInfoTitle.Font = new System.Drawing.Font("Microsoft YaHei UI", 13F, System.Drawing.FontStyle.Bold);
            label_FWInfoTitle.ForeColor = System.Drawing.Color.FromArgb(50, 50, 50);
            label_FWInfoTitle.Location = new System.Drawing.Point(27, 285);
            label_FWInfoTitle.AutoSize = true;
            this.Controls.Add(label_FWInfoTitle);

            // Size
            var label_SizeTitle = new System.Windows.Forms.Label();
            label_SizeTitle.Text = "Size:";
            label_SizeTitle.Font = new System.Drawing.Font("Microsoft YaHei UI", 10F);
            label_SizeTitle.ForeColor = System.Drawing.Color.FromArgb(170, 170, 170);
            label_SizeTitle.Location = new System.Drawing.Point(27, 313);
            label_SizeTitle.AutoSize = true;
            this.Controls.Add(label_SizeTitle);

            this.label_FWSize.Font = new System.Drawing.Font("Consolas", 10F);
            this.label_FWSize.ForeColor = System.Drawing.Color.FromArgb(50, 50, 50);
            this.label_FWSize.Location = new System.Drawing.Point(72, 313);
            this.label_FWSize.AutoSize = true;
            this.label_FWSize.TabIndex = 20;
            this.label_FWSize.Text = "--";
            this.Controls.Add(this.label_FWSize);

            // CRC
            var label_CRCTitle = new System.Windows.Forms.Label();
            label_CRCTitle.Text = "CRC:";
            label_CRCTitle.Font = new System.Drawing.Font("Microsoft YaHei UI", 10F);
            label_CRCTitle.ForeColor = System.Drawing.Color.FromArgb(170, 170, 170);
            label_CRCTitle.Location = new System.Drawing.Point(197, 313);
            label_CRCTitle.AutoSize = true;
            this.Controls.Add(label_CRCTitle);

            this.label_FWCRC.Font = new System.Drawing.Font("Consolas", 10F);
            this.label_FWCRC.ForeColor = System.Drawing.Color.FromArgb(50, 50, 50);
            this.label_FWCRC.Location = new System.Drawing.Point(237, 313);
            this.label_FWCRC.AutoSize = true;
            this.label_FWCRC.TabIndex = 21;
            this.label_FWCRC.Text = "--";
            this.Controls.Add(this.label_FWCRC);

            // Target
            var label_TargetTitle = new System.Windows.Forms.Label();
            label_TargetTitle.Text = "Target:";
            label_TargetTitle.Font = new System.Drawing.Font("Microsoft YaHei UI", 10F);
            label_TargetTitle.ForeColor = System.Drawing.Color.FromArgb(170, 170, 170);
            label_TargetTitle.Location = new System.Drawing.Point(347, 313);
            label_TargetTitle.AutoSize = true;
            this.Controls.Add(label_TargetTitle);

            this.label_FWTarget.Font = new System.Drawing.Font("Consolas", 10F, System.Drawing.FontStyle.Bold);
            this.label_FWTarget.ForeColor = System.Drawing.Color.FromArgb(0, 120, 212);
            this.label_FWTarget.Location = new System.Drawing.Point(402, 313);
            this.label_FWTarget.AutoSize = true;
            this.label_FWTarget.TabIndex = 22;
            this.label_FWTarget.Text = "0x08010000";
            this.Controls.Add(this.label_FWTarget);

            // Status
            var label_StatusTitle = new System.Windows.Forms.Label();
            label_StatusTitle.Text = "Status:";
            label_StatusTitle.Font = new System.Drawing.Font("Microsoft YaHei UI", 10F);
            label_StatusTitle.ForeColor = System.Drawing.Color.FromArgb(170, 170, 170);
            label_StatusTitle.Location = new System.Drawing.Point(27, 341);
            label_StatusTitle.AutoSize = true;
            this.Controls.Add(label_StatusTitle);

            this.label_FWStatus.Font = new System.Drawing.Font("Microsoft YaHei UI", 10F, System.Drawing.FontStyle.Bold);
            this.label_FWStatus.ForeColor = System.Drawing.Color.FromArgb(170, 170, 170);
            this.label_FWStatus.Location = new System.Drawing.Point(72, 341);
            this.label_FWStatus.AutoSize = true;
            this.label_FWStatus.TabIndex = 23;
            this.label_FWStatus.Text = "--";
            this.Controls.Add(this.label_FWStatus);

            // ==================== Progress Section ====================
            var label_ProgressTitle = new System.Windows.Forms.Label();
            label_ProgressTitle.Text = "Progress";
            label_ProgressTitle.Font = new System.Drawing.Font("Microsoft YaHei UI", 13F, System.Drawing.FontStyle.Bold);
            label_ProgressTitle.ForeColor = System.Drawing.Color.FromArgb(50, 50, 50);
            label_ProgressTitle.Location = new System.Drawing.Point(27, 373);
            label_ProgressTitle.AutoSize = true;
            this.Controls.Add(label_ProgressTitle);

            this.progressBar.Location = new System.Drawing.Point(27, 401);
            this.progressBar.Name = "progressBar";
            this.progressBar.Size = new System.Drawing.Size(470, 16);
            this.progressBar.TabIndex = 24;
            this.progressBar.Style = System.Windows.Forms.ProgressBarStyle.Continuous;
            this.Controls.Add(this.progressBar);

            this.label_Progress.Font = new System.Drawing.Font("Microsoft YaHei UI", 10F, System.Drawing.FontStyle.Bold);
            this.label_Progress.ForeColor = System.Drawing.Color.FromArgb(50, 50, 50);
            this.label_Progress.Location = new System.Drawing.Point(27, 421);
            this.label_Progress.Name = "label_Progress";
            this.label_Progress.AutoSize = true;
            this.label_Progress.TabIndex = 25;
            this.label_Progress.Text = "Ready";
            this.Controls.Add(this.label_Progress);

            this.label_Speed.Font = new System.Drawing.Font("Consolas", 9F);
            this.label_Speed.ForeColor = System.Drawing.Color.FromArgb(170, 170, 170);
            this.label_Speed.Location = new System.Drawing.Point(412, 421);
            this.label_Speed.Name = "label_Speed";
            this.label_Speed.AutoSize = true;
            this.label_Speed.TabIndex = 26;
            this.label_Speed.Text = "0 B/s";
            this.Controls.Add(this.label_Speed);

            // ==================== Action Buttons ====================
            // Upload - Primary button (green)
            this.button_Upload.BackColor = System.Drawing.Color.FromArgb(16, 124, 16);
            this.button_Upload.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.button_Upload.Font = new System.Drawing.Font("Microsoft YaHei UI", 10F, System.Drawing.FontStyle.Bold);
            this.button_Upload.ForeColor = System.Drawing.Color.White;
            this.button_Upload.Location = new System.Drawing.Point(12, 453);
            this.button_Upload.Name = "button_Upload";
            this.button_Upload.Size = new System.Drawing.Size(90, 36);
            this.button_Upload.TabIndex = 27;
            this.button_Upload.Text = "Upload";
            this.button_Upload.UseVisualStyleBackColor = false;
            this.button_Upload.Click += new System.EventHandler(this.button_Upload_Click);
            this.Controls.Add(this.button_Upload);

            // Reset - Warning (orange)
            this.button_Reset.BackColor = System.Drawing.Color.Transparent;
            this.button_Reset.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.button_Reset.Font = new System.Drawing.Font("Microsoft YaHei UI", 10F, System.Drawing.FontStyle.Bold);
            this.button_Reset.ForeColor = System.Drawing.Color.FromArgb(255, 140, 0);
            this.button_Reset.Location = new System.Drawing.Point(114, 453);
            this.button_Reset.Name = "button_Reset";
            this.button_Reset.Size = new System.Drawing.Size(90, 36);
            this.button_Reset.TabIndex = 28;
            this.button_Reset.Text = "Reset";
            this.button_Reset.UseVisualStyleBackColor = false;
            this.button_Reset.Click += new System.EventHandler(this.button_Reset_Click);
            this.Controls.Add(this.button_Reset);

            // Clear Log - Tertiary (text only)
            this.button_ClearLog.BackColor = System.Drawing.Color.Transparent;
            this.button_ClearLog.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.button_ClearLog.Font = new System.Drawing.Font("Microsoft YaHei UI", 9F);
            this.button_ClearLog.ForeColor = System.Drawing.Color.FromArgb(170, 170, 170);
            this.button_ClearLog.Location = new System.Drawing.Point(432, 456);
            this.button_ClearLog.Name = "button_ClearLog";
            this.button_ClearLog.Size = new System.Drawing.Size(80, 26);
            this.button_ClearLog.TabIndex = 30;
            this.button_ClearLog.Text = "Clear Log";
            this.button_ClearLog.UseVisualStyleBackColor = false;
            this.button_ClearLog.Click += new System.EventHandler(this.button_ClearLog_Click);
            this.Controls.Add(this.button_ClearLog);

            // ==================== Log Area ====================
            var label_LogTitle = new System.Windows.Forms.Label();
            label_LogTitle.Text = "Log Output";
            label_LogTitle.Font = new System.Drawing.Font("Microsoft YaHei UI", 13F, System.Drawing.FontStyle.Bold);
            label_LogTitle.ForeColor = System.Drawing.Color.FromArgb(50, 50, 50);
            label_LogTitle.Location = new System.Drawing.Point(27, 496);
            label_LogTitle.AutoSize = true;
            this.Controls.Add(label_LogTitle);

            this.textBox_Log.BackColor = System.Drawing.Color.FromArgb(250, 250, 250);
            this.textBox_Log.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.textBox_Log.Font = new System.Drawing.Font("Consolas", 10F);
            this.textBox_Log.ForeColor = System.Drawing.Color.FromArgb(50, 50, 50);
            this.textBox_Log.Location = new System.Drawing.Point(12, 521);
            this.textBox_Log.Multiline = true;
            this.textBox_Log.Name = "textBox_Log";
            this.textBox_Log.ReadOnly = true;
            this.textBox_Log.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.textBox_Log.Size = new System.Drawing.Size(500, 200);
            this.textBox_Log.TabIndex = 31;
            this.textBox_Log.Text = "";
            this.Controls.Add(this.textBox_Log);

            // ==================== Form Settings ====================
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 17F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.FromArgb(240, 240, 240);
            this.ClientSize = new System.Drawing.Size(524, 748);
            this.Font = new System.Drawing.Font("Microsoft YaHei UI", 9F);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.MaximizeBox = false;
            this.Name = "Form1";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "MCU_BOOT Tool v1.0.0";
            this.Load += new System.EventHandler(this.Form1_Load);

            this.ResumeLayout(false);
        }
    }
}
