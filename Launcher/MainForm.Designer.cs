
namespace Launcher
{
	partial class MainForm
	{
		/// <summary>
		/// Designer variable used to keep track of non-visual components.
		/// </summary>
		private System.ComponentModel.IContainer components = null;
		
		/// <summary>
		/// Disposes resources used by the form.
		/// </summary>
		/// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
		protected override void Dispose(bool disposing)
		{
			if (disposing) {
				if (components != null) {
					components.Dispose();
				}
			}
			base.Dispose(disposing);
		}
		
		/// <summary>
		/// This method is required for Windows Forms designer support.
		/// Do not change the method contents inside the source code editor. The Forms designer might
		/// not be able to load this method if it was changed manually.
		/// </summary>
		private void InitializeComponent()
		{
			this.components = new System.ComponentModel.Container();
			this.toolTip = new System.Windows.Forms.ToolTip(this.components);
			this.tabMinecraftNet = new System.Windows.Forms.TabPage();
			this.tabMC = new System.Windows.Forms.TabControl();
			this.tabMCSignIn = new System.Windows.Forms.TabPage();
			this.prgMCStatus = new System.Windows.Forms.ProgressBar();
			this.lblMCStatusLabel = new System.Windows.Forms.Label();
			this.lblMCStatus = new System.Windows.Forms.Label();
			this.lbMCUser = new System.Windows.Forms.Label();
			this.btnMCSignIn = new System.Windows.Forms.Button();
			this.txtMCUser = new System.Windows.Forms.TextBox();
			this.txtMCPassword = new System.Windows.Forms.TextBox();
			this.lblMCPass = new System.Windows.Forms.Label();
			this.tabMCServers = new System.Windows.Forms.TabPage();
			this.cbMCHideInvalid = new System.Windows.Forms.CheckBox();
			this.txtMCHash = new System.Windows.Forms.TextBox();
			this.lblMCHash = new System.Windows.Forms.Label();
			this.btnMCConnect = new System.Windows.Forms.Button();
			this.tblMCServers = new System.Windows.Forms.ListView();
			this.columnHeader1 = new System.Windows.Forms.ColumnHeader();
			this.columnHeader2 = new System.Windows.Forms.ColumnHeader();
			this.columnHeader3 = new System.Windows.Forms.ColumnHeader();
			this.columnHeader4 = new System.Windows.Forms.ColumnHeader();
			this.cbMCHideEmpty = new System.Windows.Forms.CheckBox();
			this.txtMCSearch = new System.Windows.Forms.TextBox();
			this.lblMCSearch = new System.Windows.Forms.Label();
			this.tabLocal = new System.Windows.Forms.TabPage();
			this.btnLanConnect = new System.Windows.Forms.Button();
			this.cbLocalSkinServerCC = new System.Windows.Forms.CheckBox();
			this.txtLanPort = new System.Windows.Forms.TextBox();
			this.lblLanPort = new System.Windows.Forms.Label();
			this.txtLanIP = new System.Windows.Forms.TextBox();
			this.lblLanIP = new System.Windows.Forms.Label();
			this.lblLanUser = new System.Windows.Forms.Label();
			this.txtLanUser = new System.Windows.Forms.TextBox();
			this.tabs = new System.Windows.Forms.TabControl();
			this.tabClassicubeNet = new System.Windows.Forms.TabPage();
			this.tabCC = new System.Windows.Forms.TabControl();
			this.tabCCSignIn = new System.Windows.Forms.TabPage();
			this.prgCCStatus = new System.Windows.Forms.ProgressBar();
			this.lblCCStatusLabel = new System.Windows.Forms.Label();
			this.lblCCStatus = new System.Windows.Forms.Label();
			this.lblCCUser = new System.Windows.Forms.Label();
			this.btnCCSignIn = new System.Windows.Forms.Button();
			this.txtCCUser = new System.Windows.Forms.TextBox();
			this.txtCCPassword = new System.Windows.Forms.TextBox();
			this.lblCCPassword = new System.Windows.Forms.Label();
			this.tabCCServers = new System.Windows.Forms.TabPage();
			this.txtCCHash = new System.Windows.Forms.TextBox();
			this.lblCCPlayUrl = new System.Windows.Forms.Label();
			this.btnCCConnect = new System.Windows.Forms.Button();
			this.tblCCServers = new System.Windows.Forms.ListView();
			this.colCCName = new System.Windows.Forms.ColumnHeader();
			this.colCCPlayers = new System.Windows.Forms.ColumnHeader();
			this.colCCMaxPlayers = new System.Windows.Forms.ColumnHeader();
			this.colCCUptime = new System.Windows.Forms.ColumnHeader();
			this.cbCCHideEmpty = new System.Windows.Forms.CheckBox();
			this.txtCCSearch = new System.Windows.Forms.TextBox();
			this.lblCCSearch = new System.Windows.Forms.Label();
			this.tabMinecraftNet.SuspendLayout();
			this.tabMC.SuspendLayout();
			this.tabMCSignIn.SuspendLayout();
			this.tabMCServers.SuspendLayout();
			this.tabLocal.SuspendLayout();
			this.tabs.SuspendLayout();
			this.tabClassicubeNet.SuspendLayout();
			this.tabCC.SuspendLayout();
			this.tabCCSignIn.SuspendLayout();
			this.tabCCServers.SuspendLayout();
			this.SuspendLayout();
			// 
			// tabMinecraftNet
			// 
			this.tabMinecraftNet.Controls.Add(this.tabMC);
			this.tabMinecraftNet.Location = new System.Drawing.Point(4, 22);
			this.tabMinecraftNet.Name = "tabMinecraftNet";
			this.tabMinecraftNet.Padding = new System.Windows.Forms.Padding(3);
			this.tabMinecraftNet.Size = new System.Drawing.Size(474, 440);
			this.tabMinecraftNet.TabIndex = 1;
			this.tabMinecraftNet.Text = "minecraft.net";
			this.tabMinecraftNet.UseVisualStyleBackColor = true;
			// 
			// tabMC
			// 
			this.tabMC.Controls.Add(this.tabMCSignIn);
			this.tabMC.Controls.Add(this.tabMCServers);
			this.tabMC.Location = new System.Drawing.Point(0, 0);
			this.tabMC.Name = "tabMC";
			this.tabMC.SelectedIndex = 0;
			this.tabMC.Size = new System.Drawing.Size(482, 444);
			this.tabMC.TabIndex = 2;
			// 
			// tabMCSignIn
			// 
			this.tabMCSignIn.Controls.Add(this.prgMCStatus);
			this.tabMCSignIn.Controls.Add(this.lblMCStatusLabel);
			this.tabMCSignIn.Controls.Add(this.lblMCStatus);
			this.tabMCSignIn.Controls.Add(this.lbMCUser);
			this.tabMCSignIn.Controls.Add(this.btnMCSignIn);
			this.tabMCSignIn.Controls.Add(this.txtMCUser);
			this.tabMCSignIn.Controls.Add(this.txtMCPassword);
			this.tabMCSignIn.Controls.Add(this.lblMCPass);
			this.tabMCSignIn.Location = new System.Drawing.Point(4, 22);
			this.tabMCSignIn.Name = "tabMCSignIn";
			this.tabMCSignIn.Padding = new System.Windows.Forms.Padding(3);
			this.tabMCSignIn.Size = new System.Drawing.Size(474, 418);
			this.tabMCSignIn.TabIndex = 0;
			this.tabMCSignIn.Text = "Sign in";
			this.tabMCSignIn.UseVisualStyleBackColor = true;
			// 
			// prgMCStatus
			// 
			this.prgMCStatus.ForeColor = System.Drawing.SystemColors.Desktop;
			this.prgMCStatus.Location = new System.Drawing.Point(10, 200);
			this.prgMCStatus.Name = "prgMCStatus";
			this.prgMCStatus.Size = new System.Drawing.Size(200, 20);
			this.prgMCStatus.Style = System.Windows.Forms.ProgressBarStyle.Continuous;
			this.prgMCStatus.TabIndex = 8;
			// 
			// lblMCStatusLabel
			// 
			this.lblMCStatusLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblMCStatusLabel.Location = new System.Drawing.Point(10, 180);
			this.lblMCStatusLabel.Name = "lblMCStatusLabel";
			this.lblMCStatusLabel.Size = new System.Drawing.Size(77, 20);
			this.lblMCStatusLabel.TabIndex = 7;
			this.lblMCStatusLabel.Text = "Status:";
			// 
			// lblMCStatus
			// 
			this.lblMCStatus.AutoSize = true;
			this.lblMCStatus.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblMCStatus.Location = new System.Drawing.Point(10, 230);
			this.lblMCStatus.Name = "lblMCStatus";
			this.lblMCStatus.Size = new System.Drawing.Size(0, 17);
			this.lblMCStatus.TabIndex = 6;
			// 
			// lbMCUser
			// 
			this.lbMCUser.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lbMCUser.Location = new System.Drawing.Point(10, 10);
			this.lbMCUser.Name = "lbMCUser";
			this.lbMCUser.Size = new System.Drawing.Size(81, 20);
			this.lbMCUser.TabIndex = 0;
			this.lbMCUser.Text = "Username";
			// 
			// btnMCSignIn
			// 
			this.btnMCSignIn.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.btnMCSignIn.Location = new System.Drawing.Point(10, 120);
			this.btnMCSignIn.Name = "btnMCSignIn";
			this.btnMCSignIn.Size = new System.Drawing.Size(100, 30);
			this.btnMCSignIn.TabIndex = 4;
			this.btnMCSignIn.Text = "Sign in";
			this.btnMCSignIn.UseVisualStyleBackColor = true;
			this.btnMCSignIn.Click += new System.EventHandler(this.btnMCSignInClick);
			// 
			// txtMCUser
			// 
			this.txtMCUser.Location = new System.Drawing.Point(10, 30);
			this.txtMCUser.MaxLength = 64;
			this.txtMCUser.Name = "txtMCUser";
			this.txtMCUser.Size = new System.Drawing.Size(100, 20);
			this.txtMCUser.TabIndex = 1;
			// 
			// txtMCPassword
			// 
			this.txtMCPassword.Location = new System.Drawing.Point(10, 80);
			this.txtMCPassword.MaxLength = 64;
			this.txtMCPassword.Name = "txtMCPassword";
			this.txtMCPassword.PasswordChar = '*';
			this.txtMCPassword.Size = new System.Drawing.Size(100, 20);
			this.txtMCPassword.TabIndex = 3;
			// 
			// lblMCPass
			// 
			this.lblMCPass.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblMCPass.Location = new System.Drawing.Point(10, 60);
			this.lblMCPass.Name = "lblMCPass";
			this.lblMCPass.Size = new System.Drawing.Size(77, 20);
			this.lblMCPass.TabIndex = 2;
			this.lblMCPass.Text = "Password";
			// 
			// tabMCServers
			// 
			this.tabMCServers.Controls.Add(this.cbMCHideInvalid);
			this.tabMCServers.Controls.Add(this.txtMCHash);
			this.tabMCServers.Controls.Add(this.lblMCHash);
			this.tabMCServers.Controls.Add(this.btnMCConnect);
			this.tabMCServers.Controls.Add(this.tblMCServers);
			this.tabMCServers.Controls.Add(this.cbMCHideEmpty);
			this.tabMCServers.Controls.Add(this.txtMCSearch);
			this.tabMCServers.Controls.Add(this.lblMCSearch);
			this.tabMCServers.Location = new System.Drawing.Point(4, 22);
			this.tabMCServers.Name = "tabMCServers";
			this.tabMCServers.Padding = new System.Windows.Forms.Padding(3);
			this.tabMCServers.Size = new System.Drawing.Size(474, 418);
			this.tabMCServers.TabIndex = 1;
			this.tabMCServers.Text = "minecraft.net server";
			this.tabMCServers.UseVisualStyleBackColor = true;
			// 
			// cbMCHideInvalid
			// 
			this.cbMCHideInvalid.AutoSize = true;
			this.cbMCHideInvalid.Location = new System.Drawing.Point(230, 21);
			this.cbMCHideInvalid.Name = "cbMCHideInvalid";
			this.cbMCHideInvalid.Size = new System.Drawing.Size(118, 17);
			this.cbMCHideInvalid.TabIndex = 10;
			this.cbMCHideInvalid.Text = "Hide invalid servers";
			this.cbMCHideInvalid.UseVisualStyleBackColor = true;
			this.cbMCHideInvalid.CheckedChanged += new System.EventHandler(this.cbMCHideInvalidCheckedChanged);
			// 
			// txtMCHash
			// 
			this.txtMCHash.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.txtMCHash.BackColor = System.Drawing.SystemColors.Window;
			this.txtMCHash.Location = new System.Drawing.Point(137, 389);
			this.txtMCHash.Name = "txtMCHash";
			this.txtMCHash.Size = new System.Drawing.Size(190, 20);
			this.txtMCHash.TabIndex = 9;
			this.txtMCHash.TextChanged += new System.EventHandler(this.txtMCHashTextChanged);
			// 
			// lblMCHash
			// 
			this.lblMCHash.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.lblMCHash.AutoSize = true;
			this.lblMCHash.Location = new System.Drawing.Point(3, 392);
			this.lblMCHash.Name = "lblMCHash";
			this.lblMCHash.Size = new System.Drawing.Size(134, 13);
			this.lblMCHash.TabIndex = 8;
			this.lblMCHash.Text = "minecraft.net/classic/play/";
			// 
			// btnMCConnect
			// 
			this.btnMCConnect.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.btnMCConnect.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.btnMCConnect.Location = new System.Drawing.Point(367, 382);
			this.btnMCConnect.Name = "btnMCConnect";
			this.btnMCConnect.Size = new System.Drawing.Size(100, 30);
			this.btnMCConnect.TabIndex = 7;
			this.btnMCConnect.Text = "Connect";
			this.btnMCConnect.UseVisualStyleBackColor = true;
			this.btnMCConnect.Click += new System.EventHandler(this.btnMCConnectClick);
			// 
			// tblMCServers
			// 
			this.tblMCServers.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
									this.columnHeader1,
									this.columnHeader2,
									this.columnHeader3,
									this.columnHeader4});
			this.tblMCServers.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.tblMCServers.FullRowSelect = true;
			this.tblMCServers.GridLines = true;
			this.tblMCServers.HideSelection = false;
			this.tblMCServers.Location = new System.Drawing.Point(0, 40);
			this.tblMCServers.Name = "tblMCServers";
			this.tblMCServers.Size = new System.Drawing.Size(474, 335);
			this.tblMCServers.TabIndex = 6;
			this.tblMCServers.UseCompatibleStateImageBehavior = false;
			this.tblMCServers.View = System.Windows.Forms.View.Details;
			this.tblMCServers.ColumnClick += new System.Windows.Forms.ColumnClickEventHandler(this.tblMCServersColumnClick);
			this.tblMCServers.Click += new System.EventHandler(this.tblMCServersClick);
			// 
			// columnHeader1
			// 
			this.columnHeader1.Text = "Name";
			this.columnHeader1.Width = 100;
			// 
			// columnHeader2
			// 
			this.columnHeader2.Text = "Players";
			this.columnHeader2.Width = 90;
			// 
			// columnHeader3
			// 
			this.columnHeader3.Text = "Max players";
			this.columnHeader3.Width = 90;
			// 
			// columnHeader4
			// 
			this.columnHeader4.Text = "Uptime";
			this.columnHeader4.Width = 100;
			// 
			// cbMCHideEmpty
			// 
			this.cbMCHideEmpty.AutoSize = true;
			this.cbMCHideEmpty.Location = new System.Drawing.Point(230, 3);
			this.cbMCHideEmpty.Name = "cbMCHideEmpty";
			this.cbMCHideEmpty.Size = new System.Drawing.Size(116, 17);
			this.cbMCHideEmpty.TabIndex = 3;
			this.cbMCHideEmpty.Text = "Hide empty servers";
			this.cbMCHideEmpty.UseVisualStyleBackColor = true;
			this.cbMCHideEmpty.CheckedChanged += new System.EventHandler(this.cbMCHideEmptyCheckedChanged);
			// 
			// txtMCSearch
			// 
			this.txtMCSearch.Location = new System.Drawing.Point(80, 10);
			this.txtMCSearch.Name = "txtMCSearch";
			this.txtMCSearch.Size = new System.Drawing.Size(100, 20);
			this.txtMCSearch.TabIndex = 2;
			this.txtMCSearch.TextChanged += new System.EventHandler(this.txtMCSearchTextChanged);
			// 
			// lblMCSearch
			// 
			this.lblMCSearch.AutoSize = true;
			this.lblMCSearch.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblMCSearch.Location = new System.Drawing.Point(10, 10);
			this.lblMCSearch.Name = "lblMCSearch";
			this.lblMCSearch.Size = new System.Drawing.Size(64, 20);
			this.lblMCSearch.TabIndex = 1;
			this.lblMCSearch.Text = "Search:";
			// 
			// tabLocal
			// 
			this.tabLocal.Controls.Add(this.btnLanConnect);
			this.tabLocal.Controls.Add(this.cbLocalSkinServerCC);
			this.tabLocal.Controls.Add(this.txtLanPort);
			this.tabLocal.Controls.Add(this.lblLanPort);
			this.tabLocal.Controls.Add(this.txtLanIP);
			this.tabLocal.Controls.Add(this.lblLanIP);
			this.tabLocal.Controls.Add(this.lblLanUser);
			this.tabLocal.Controls.Add(this.txtLanUser);
			this.tabLocal.Location = new System.Drawing.Point(4, 22);
			this.tabLocal.Name = "tabLocal";
			this.tabLocal.Padding = new System.Windows.Forms.Padding(3);
			this.tabLocal.Size = new System.Drawing.Size(474, 440);
			this.tabLocal.TabIndex = 0;
			this.tabLocal.Text = "Local server";
			this.tabLocal.UseVisualStyleBackColor = true;
			// 
			// btnLanConnect
			// 
			this.btnLanConnect.AutoSize = true;
			this.btnLanConnect.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.btnLanConnect.Location = new System.Drawing.Point(10, 200);
			this.btnLanConnect.Name = "btnLanConnect";
			this.btnLanConnect.Size = new System.Drawing.Size(86, 30);
			this.btnLanConnect.TabIndex = 7;
			this.btnLanConnect.Text = "Connect";
			this.btnLanConnect.UseVisualStyleBackColor = true;
			this.btnLanConnect.Click += new System.EventHandler(this.BtnLanConnectClick);
			// 
			// cbLocalSkinServerCC
			// 
			this.cbLocalSkinServerCC.Location = new System.Drawing.Point(10, 160);
			this.cbLocalSkinServerCC.Name = "cbLocalSkinServerCC";
			this.cbLocalSkinServerCC.Size = new System.Drawing.Size(180, 20);
			this.cbLocalSkinServerCC.TabIndex = 6;
			this.cbLocalSkinServerCC.Text = "Use Classicube.net for skins";
			this.cbLocalSkinServerCC.UseVisualStyleBackColor = true;
			// 
			// txtLanPort
			// 
			this.txtLanPort.Location = new System.Drawing.Point(10, 130);
			this.txtLanPort.MaxLength = 6;
			this.txtLanPort.Name = "txtLanPort";
			this.txtLanPort.Size = new System.Drawing.Size(100, 20);
			this.txtLanPort.TabIndex = 5;
			this.txtLanPort.Text = "25565";
			// 
			// lblLanPort
			// 
			this.lblLanPort.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblLanPort.Location = new System.Drawing.Point(10, 110);
			this.lblLanPort.Name = "lblLanPort";
			this.lblLanPort.Size = new System.Drawing.Size(100, 20);
			this.lblLanPort.TabIndex = 4;
			this.lblLanPort.Text = "Port number";
			// 
			// txtLanIP
			// 
			this.txtLanIP.Location = new System.Drawing.Point(10, 80);
			this.txtLanIP.Name = "txtLanIP";
			this.txtLanIP.Size = new System.Drawing.Size(100, 20);
			this.txtLanIP.TabIndex = 3;
			this.txtLanIP.Text = "127.0.0.1";
			// 
			// lblLanIP
			// 
			this.lblLanIP.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblLanIP.Location = new System.Drawing.Point(10, 60);
			this.lblLanIP.Name = "lblLanIP";
			this.lblLanIP.Size = new System.Drawing.Size(150, 20);
			this.lblLanIP.TabIndex = 2;
			this.lblLanIP.Text = "Local IP address";
			// 
			// lblLanUser
			// 
			this.lblLanUser.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblLanUser.Location = new System.Drawing.Point(10, 10);
			this.lblLanUser.Name = "lblLanUser";
			this.lblLanUser.Size = new System.Drawing.Size(100, 20);
			this.lblLanUser.TabIndex = 1;
			this.lblLanUser.Text = "Username";
			// 
			// txtLanUser
			// 
			this.txtLanUser.Location = new System.Drawing.Point(10, 30);
			this.txtLanUser.MaxLength = 64;
			this.txtLanUser.Name = "txtLanUser";
			this.txtLanUser.Size = new System.Drawing.Size(100, 20);
			this.txtLanUser.TabIndex = 0;
			// 
			// tabs
			// 
			this.tabs.Controls.Add(this.tabLocal);
			this.tabs.Controls.Add(this.tabMinecraftNet);
			this.tabs.Controls.Add(this.tabClassicubeNet);
			this.tabs.Location = new System.Drawing.Point(-1, 0);
			this.tabs.Name = "tabs";
			this.tabs.SelectedIndex = 0;
			this.tabs.Size = new System.Drawing.Size(482, 466);
			this.tabs.TabIndex = 0;
			// 
			// tabClassicubeNet
			// 
			this.tabClassicubeNet.Controls.Add(this.tabCC);
			this.tabClassicubeNet.Location = new System.Drawing.Point(4, 22);
			this.tabClassicubeNet.Name = "tabClassicubeNet";
			this.tabClassicubeNet.Size = new System.Drawing.Size(474, 440);
			this.tabClassicubeNet.TabIndex = 2;
			this.tabClassicubeNet.Text = "classicube.net";
			this.tabClassicubeNet.UseVisualStyleBackColor = true;
			// 
			// tabCC
			// 
			this.tabCC.Controls.Add(this.tabCCSignIn);
			this.tabCC.Controls.Add(this.tabCCServers);
			this.tabCC.Location = new System.Drawing.Point(0, 0);
			this.tabCC.Name = "tabCC";
			this.tabCC.SelectedIndex = 0;
			this.tabCC.Size = new System.Drawing.Size(482, 444);
			this.tabCC.TabIndex = 1;
			// 
			// tabCCSignIn
			// 
			this.tabCCSignIn.Controls.Add(this.prgCCStatus);
			this.tabCCSignIn.Controls.Add(this.lblCCStatusLabel);
			this.tabCCSignIn.Controls.Add(this.lblCCStatus);
			this.tabCCSignIn.Controls.Add(this.lblCCUser);
			this.tabCCSignIn.Controls.Add(this.btnCCSignIn);
			this.tabCCSignIn.Controls.Add(this.txtCCUser);
			this.tabCCSignIn.Controls.Add(this.txtCCPassword);
			this.tabCCSignIn.Controls.Add(this.lblCCPassword);
			this.tabCCSignIn.Location = new System.Drawing.Point(4, 22);
			this.tabCCSignIn.Name = "tabCCSignIn";
			this.tabCCSignIn.Padding = new System.Windows.Forms.Padding(3);
			this.tabCCSignIn.Size = new System.Drawing.Size(474, 418);
			this.tabCCSignIn.TabIndex = 0;
			this.tabCCSignIn.Text = "Sign in";
			this.tabCCSignIn.UseVisualStyleBackColor = true;
			// 
			// prgCCStatus
			// 
			this.prgCCStatus.ForeColor = System.Drawing.SystemColors.Desktop;
			this.prgCCStatus.Location = new System.Drawing.Point(10, 200);
			this.prgCCStatus.Name = "prgCCStatus";
			this.prgCCStatus.Size = new System.Drawing.Size(200, 20);
			this.prgCCStatus.Style = System.Windows.Forms.ProgressBarStyle.Continuous;
			this.prgCCStatus.TabIndex = 8;
			// 
			// lblCCStatusLabel
			// 
			this.lblCCStatusLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblCCStatusLabel.Location = new System.Drawing.Point(10, 180);
			this.lblCCStatusLabel.Name = "lblCCStatusLabel";
			this.lblCCStatusLabel.Size = new System.Drawing.Size(77, 20);
			this.lblCCStatusLabel.TabIndex = 7;
			this.lblCCStatusLabel.Text = "Status:";
			// 
			// lblCCStatus
			// 
			this.lblCCStatus.AutoSize = true;
			this.lblCCStatus.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblCCStatus.Location = new System.Drawing.Point(10, 230);
			this.lblCCStatus.Name = "lblCCStatus";
			this.lblCCStatus.Size = new System.Drawing.Size(0, 17);
			this.lblCCStatus.TabIndex = 6;
			// 
			// lblCCUser
			// 
			this.lblCCUser.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblCCUser.Location = new System.Drawing.Point(10, 10);
			this.lblCCUser.Name = "lblCCUser";
			this.lblCCUser.Size = new System.Drawing.Size(81, 20);
			this.lblCCUser.TabIndex = 0;
			this.lblCCUser.Text = "Username";
			// 
			// btnCCSignIn
			// 
			this.btnCCSignIn.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.btnCCSignIn.Location = new System.Drawing.Point(10, 120);
			this.btnCCSignIn.Name = "btnCCSignIn";
			this.btnCCSignIn.Size = new System.Drawing.Size(100, 30);
			this.btnCCSignIn.TabIndex = 4;
			this.btnCCSignIn.Text = "Sign in";
			this.btnCCSignIn.UseVisualStyleBackColor = true;
			this.btnCCSignIn.Click += new System.EventHandler(this.btnCCSignInClick);
			// 
			// txtCCUser
			// 
			this.txtCCUser.Location = new System.Drawing.Point(10, 30);
			this.txtCCUser.MaxLength = 64;
			this.txtCCUser.Name = "txtCCUser";
			this.txtCCUser.Size = new System.Drawing.Size(100, 20);
			this.txtCCUser.TabIndex = 1;
			// 
			// txtCCPassword
			// 
			this.txtCCPassword.Location = new System.Drawing.Point(10, 80);
			this.txtCCPassword.MaxLength = 64;
			this.txtCCPassword.Name = "txtCCPassword";
			this.txtCCPassword.PasswordChar = '*';
			this.txtCCPassword.Size = new System.Drawing.Size(100, 20);
			this.txtCCPassword.TabIndex = 3;
			// 
			// lblCCPassword
			// 
			this.lblCCPassword.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblCCPassword.Location = new System.Drawing.Point(10, 60);
			this.lblCCPassword.Name = "lblCCPassword";
			this.lblCCPassword.Size = new System.Drawing.Size(77, 20);
			this.lblCCPassword.TabIndex = 2;
			this.lblCCPassword.Text = "Password";
			// 
			// tabCCServers
			// 
			this.tabCCServers.Controls.Add(this.txtCCHash);
			this.tabCCServers.Controls.Add(this.lblCCPlayUrl);
			this.tabCCServers.Controls.Add(this.btnCCConnect);
			this.tabCCServers.Controls.Add(this.tblCCServers);
			this.tabCCServers.Controls.Add(this.cbCCHideEmpty);
			this.tabCCServers.Controls.Add(this.txtCCSearch);
			this.tabCCServers.Controls.Add(this.lblCCSearch);
			this.tabCCServers.Location = new System.Drawing.Point(4, 22);
			this.tabCCServers.Name = "tabCCServers";
			this.tabCCServers.Padding = new System.Windows.Forms.Padding(3);
			this.tabCCServers.Size = new System.Drawing.Size(474, 418);
			this.tabCCServers.TabIndex = 1;
			this.tabCCServers.Text = "classicube.net server";
			this.tabCCServers.UseVisualStyleBackColor = true;
			// 
			// txtCCHash
			// 
			this.txtCCHash.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.txtCCHash.BackColor = System.Drawing.SystemColors.Window;
			this.txtCCHash.Location = new System.Drawing.Point(170, 389);
			this.txtCCHash.Name = "txtCCHash";
			this.txtCCHash.Size = new System.Drawing.Size(190, 20);
			this.txtCCHash.TabIndex = 9;
			this.txtCCHash.TextChanged += new System.EventHandler(this.txtCCHashTextChanged);
			// 
			// lblCCPlayUrl
			// 
			this.lblCCPlayUrl.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.lblCCPlayUrl.AutoSize = true;
			this.lblCCPlayUrl.Location = new System.Drawing.Point(3, 392);
			this.lblCCPlayUrl.Name = "lblCCPlayUrl";
			this.lblCCPlayUrl.Size = new System.Drawing.Size(165, 13);
			this.lblCCPlayUrl.TabIndex = 8;
			this.lblCCPlayUrl.Text = "www.classicube.net/server/play/";
			// 
			// btnCCConnect
			// 
			this.btnCCConnect.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.btnCCConnect.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.btnCCConnect.Location = new System.Drawing.Point(367, 382);
			this.btnCCConnect.Name = "btnCCConnect";
			this.btnCCConnect.Size = new System.Drawing.Size(100, 30);
			this.btnCCConnect.TabIndex = 7;
			this.btnCCConnect.Text = "Connect";
			this.btnCCConnect.UseVisualStyleBackColor = true;
			this.btnCCConnect.Click += new System.EventHandler(this.btnCCConnectClick);
			// 
			// tblCCServers
			// 
			this.tblCCServers.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
									this.colCCName,
									this.colCCPlayers,
									this.colCCMaxPlayers,
									this.colCCUptime});
			this.tblCCServers.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.tblCCServers.FullRowSelect = true;
			this.tblCCServers.GridLines = true;
			this.tblCCServers.HideSelection = false;
			this.tblCCServers.Location = new System.Drawing.Point(0, 40);
			this.tblCCServers.Name = "tblCCServers";
			this.tblCCServers.Size = new System.Drawing.Size(474, 335);
			this.tblCCServers.TabIndex = 6;
			this.tblCCServers.UseCompatibleStateImageBehavior = false;
			this.tblCCServers.View = System.Windows.Forms.View.Details;
			this.tblCCServers.ColumnClick += new System.Windows.Forms.ColumnClickEventHandler(this.tblCCServersColumnClick);
			this.tblCCServers.Click += new System.EventHandler(this.tblCCServersClick);
			// 
			// colCCName
			// 
			this.colCCName.Text = "Name";
			this.colCCName.Width = 100;
			// 
			// colCCPlayers
			// 
			this.colCCPlayers.Text = "Players";
			this.colCCPlayers.Width = 90;
			// 
			// colCCMaxPlayers
			// 
			this.colCCMaxPlayers.Text = "Max players";
			this.colCCMaxPlayers.Width = 90;
			// 
			// colCCUptime
			// 
			this.colCCUptime.Text = "Uptime";
			this.colCCUptime.Width = 100;
			// 
			// cbCCHideEmpty
			// 
			this.cbCCHideEmpty.AutoSize = true;
			this.cbCCHideEmpty.Location = new System.Drawing.Point(230, 3);
			this.cbCCHideEmpty.Name = "cbCCHideEmpty";
			this.cbCCHideEmpty.Size = new System.Drawing.Size(116, 17);
			this.cbCCHideEmpty.TabIndex = 3;
			this.cbCCHideEmpty.Text = "Hide empty servers";
			this.cbCCHideEmpty.UseVisualStyleBackColor = true;
			this.cbCCHideEmpty.CheckedChanged += new System.EventHandler(this.cbCCHideEmptyCheckedChanged);
			// 
			// txtCCSearch
			// 
			this.txtCCSearch.Location = new System.Drawing.Point(80, 10);
			this.txtCCSearch.Name = "txtCCSearch";
			this.txtCCSearch.Size = new System.Drawing.Size(100, 20);
			this.txtCCSearch.TabIndex = 2;
			this.txtCCSearch.TextChanged += new System.EventHandler(this.txtCCSearchTextChanged);
			// 
			// lblCCSearch
			// 
			this.lblCCSearch.AutoSize = true;
			this.lblCCSearch.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblCCSearch.Location = new System.Drawing.Point(10, 10);
			this.lblCCSearch.Name = "lblCCSearch";
			this.lblCCSearch.Size = new System.Drawing.Size(64, 20);
			this.lblCCSearch.TabIndex = 1;
			this.lblCCSearch.Text = "Search:";
			// 
			// MainForm
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(484, 462);
			this.Controls.Add(this.tabs);
			this.Name = "MainForm";
			this.Text = "ClassicalSharp Launcher 0.5";
			this.ResizeEnd += new System.EventHandler(this.MainFormResizeEnd);
			this.tabMinecraftNet.ResumeLayout(false);
			this.tabMC.ResumeLayout(false);
			this.tabMCSignIn.ResumeLayout(false);
			this.tabMCSignIn.PerformLayout();
			this.tabMCServers.ResumeLayout(false);
			this.tabMCServers.PerformLayout();
			this.tabLocal.ResumeLayout(false);
			this.tabLocal.PerformLayout();
			this.tabs.ResumeLayout(false);
			this.tabClassicubeNet.ResumeLayout(false);
			this.tabCC.ResumeLayout(false);
			this.tabCCSignIn.ResumeLayout(false);
			this.tabCCSignIn.PerformLayout();
			this.tabCCServers.ResumeLayout(false);
			this.tabCCServers.PerformLayout();
			this.ResumeLayout(false);
		}
		private System.Windows.Forms.CheckBox cbMCHideInvalid;
		private System.Windows.Forms.Label lblMCSearch;
		private System.Windows.Forms.TextBox txtMCSearch;
		private System.Windows.Forms.CheckBox cbMCHideEmpty;
		private System.Windows.Forms.ColumnHeader columnHeader4;
		private System.Windows.Forms.ColumnHeader columnHeader3;
		private System.Windows.Forms.ColumnHeader columnHeader2;
		private System.Windows.Forms.ColumnHeader columnHeader1;
		private System.Windows.Forms.ListView tblMCServers;
		private System.Windows.Forms.Button btnMCConnect;
		private System.Windows.Forms.Label lblMCHash;
		private System.Windows.Forms.TextBox txtMCHash;
		private System.Windows.Forms.TabPage tabMCServers;
		private System.Windows.Forms.Label lblMCPass;
		private System.Windows.Forms.TextBox txtMCPassword;
		private System.Windows.Forms.TextBox txtMCUser;
		private System.Windows.Forms.Button btnMCSignIn;
		private System.Windows.Forms.Label lbMCUser;
		private System.Windows.Forms.Label lblMCStatus;
		private System.Windows.Forms.Label lblMCStatusLabel;
		private System.Windows.Forms.ProgressBar prgMCStatus;
		private System.Windows.Forms.TabPage tabMCSignIn;
		private System.Windows.Forms.TabControl tabMC;
		private System.Windows.Forms.TextBox txtCCHash;
		private System.Windows.Forms.Label lblCCPlayUrl;
		private System.Windows.Forms.Button btnCCConnect;
		private System.Windows.Forms.Label lblCCSearch;
		private System.Windows.Forms.TextBox txtCCSearch;
		private System.Windows.Forms.ColumnHeader colCCUptime;
		private System.Windows.Forms.ColumnHeader colCCMaxPlayers;
		private System.Windows.Forms.ColumnHeader colCCPlayers;
		private System.Windows.Forms.ColumnHeader colCCName;
		private System.Windows.Forms.ListView tblCCServers;
		private System.Windows.Forms.CheckBox cbCCHideEmpty;
		private System.Windows.Forms.TabPage tabCCServers;
		private System.Windows.Forms.Label lblCCPassword;
		private System.Windows.Forms.TextBox txtCCPassword;
		private System.Windows.Forms.TextBox txtCCUser;
		private System.Windows.Forms.Button btnCCSignIn;
		private System.Windows.Forms.Label lblCCUser;
		private System.Windows.Forms.Label lblCCStatus;
		private System.Windows.Forms.Label lblCCStatusLabel;
		private System.Windows.Forms.ProgressBar prgCCStatus;
		private System.Windows.Forms.TabPage tabCCSignIn;
		private System.Windows.Forms.TabControl tabCC;
		
		private System.Windows.Forms.TabPage tabClassicubeNet;
		private System.Windows.Forms.TextBox txtLanUser;
		private System.Windows.Forms.Label lblLanUser;
		private System.Windows.Forms.Label lblLanIP;
		private System.Windows.Forms.TextBox txtLanIP;
		private System.Windows.Forms.Label lblLanPort;
		private System.Windows.Forms.TextBox txtLanPort;
		private System.Windows.Forms.CheckBox cbLocalSkinServerCC;
		private System.Windows.Forms.Button btnLanConnect;
		private System.Windows.Forms.TabControl tabs;
		private System.Windows.Forms.TabPage tabLocal;
		private System.Windows.Forms.TabPage tabMinecraftNet;
		private System.Windows.Forms.ToolTip toolTip;
	}
}