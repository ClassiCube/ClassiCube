
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
			this.tabMinecraftNet = new System.Windows.Forms.TabPage();
			this.lblMCdead2 = new System.Windows.Forms.Label();
			this.lblMCdead = new System.Windows.Forms.Label();
			this.tabDC = new System.Windows.Forms.TabPage();
			this.lblDChint = new System.Windows.Forms.Label();
			this.txtDCmppass = new System.Windows.Forms.TextBox();
			this.txtDCuser = new System.Windows.Forms.TextBox();
			this.txtDCport = new System.Windows.Forms.TextBox();
			this.txtDCip = new System.Windows.Forms.TextBox();
			this.lblDCmppass = new System.Windows.Forms.Label();
			this.label2 = new System.Windows.Forms.Label();
			this.cbDCccskins = new System.Windows.Forms.CheckBox();
			this.btnDCconnect = new System.Windows.Forms.Button();
			this.lblDCaddress = new System.Windows.Forms.Label();
			this.lblDCuser = new System.Windows.Forms.Label();
			this.tabs = new System.Windows.Forms.TabControl();
			this.tabClassicubeNet.SuspendLayout();
			this.tabCC.SuspendLayout();
			this.tabCCSignIn.SuspendLayout();
			this.tabCCServers.SuspendLayout();
			this.tabMinecraftNet.SuspendLayout();
			this.tabDC.SuspendLayout();
			this.tabs.SuspendLayout();
			this.SuspendLayout();
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
			// tabMinecraftNet
			// 
			this.tabMinecraftNet.Controls.Add(this.lblMCdead2);
			this.tabMinecraftNet.Controls.Add(this.lblMCdead);
			this.tabMinecraftNet.Location = new System.Drawing.Point(4, 22);
			this.tabMinecraftNet.Name = "tabMinecraftNet";
			this.tabMinecraftNet.Padding = new System.Windows.Forms.Padding(3);
			this.tabMinecraftNet.Size = new System.Drawing.Size(474, 440);
			this.tabMinecraftNet.TabIndex = 1;
			this.tabMinecraftNet.Text = "minecraft.net";
			this.tabMinecraftNet.UseVisualStyleBackColor = true;
			// 
			// lblMCdead2
			// 
			this.lblMCdead2.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblMCdead2.Location = new System.Drawing.Point(20, 62);
			this.lblMCdead2.Name = "lblMCdead2";
			this.lblMCdead2.Size = new System.Drawing.Size(300, 41);
			this.lblMCdead2.TabIndex = 1;
			this.lblMCdead2.Text = "But don\'t despair! You can sign up for an account at classicube.net.";
			// 
			// lblMCdead
			// 
			this.lblMCdead.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblMCdead.Location = new System.Drawing.Point(20, 20);
			this.lblMCdead.Name = "lblMCdead";
			this.lblMCdead.Size = new System.Drawing.Size(380, 23);
			this.lblMCdead.TabIndex = 0;
			this.lblMCdead.Text = "Classic has been removed from minecraft.net.";
			// 
			// tabDC
			// 
			this.tabDC.Controls.Add(this.lblDChint);
			this.tabDC.Controls.Add(this.txtDCmppass);
			this.tabDC.Controls.Add(this.txtDCuser);
			this.tabDC.Controls.Add(this.txtDCport);
			this.tabDC.Controls.Add(this.txtDCip);
			this.tabDC.Controls.Add(this.lblDCmppass);
			this.tabDC.Controls.Add(this.label2);
			this.tabDC.Controls.Add(this.cbDCccskins);
			this.tabDC.Controls.Add(this.btnDCconnect);
			this.tabDC.Controls.Add(this.lblDCaddress);
			this.tabDC.Controls.Add(this.lblDCuser);
			this.tabDC.Location = new System.Drawing.Point(4, 22);
			this.tabDC.Name = "tabDC";
			this.tabDC.Padding = new System.Windows.Forms.Padding(3);
			this.tabDC.Size = new System.Drawing.Size(474, 440);
			this.tabDC.TabIndex = 0;
			this.tabDC.Text = "Direct connect";
			this.tabDC.UseVisualStyleBackColor = true;
			// 
			// lblDChint
			// 
			this.lblDChint.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Italic, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblDChint.Location = new System.Drawing.Point(100, 103);
			this.lblDChint.Name = "lblDChint";
			this.lblDChint.Size = new System.Drawing.Size(250, 23);
			this.lblDChint.TabIndex = 25;
			this.lblDChint.Text = "Leave blank if connecting to a LAN server";
			// 
			// txtDCmppass
			// 
			this.txtDCmppass.Location = new System.Drawing.Point(100, 80);
			this.txtDCmppass.MaxLength = 64;
			this.txtDCmppass.Name = "txtDCmppass";
			this.txtDCmppass.Size = new System.Drawing.Size(250, 20);
			this.txtDCmppass.TabIndex = 24;
			// 
			// txtDCuser
			// 
			this.txtDCuser.Location = new System.Drawing.Point(100, 10);
			this.txtDCuser.MaxLength = 64;
			this.txtDCuser.Name = "txtDCuser";
			this.txtDCuser.Size = new System.Drawing.Size(130, 20);
			this.txtDCuser.TabIndex = 19;
			this.txtDCuser.Text = "ExampleGuy";
			// 
			// txtDCport
			// 
			this.txtDCport.Location = new System.Drawing.Point(250, 45);
			this.txtDCport.MaxLength = 6;
			this.txtDCport.Name = "txtDCport";
			this.txtDCport.Size = new System.Drawing.Size(100, 20);
			this.txtDCport.TabIndex = 18;
			this.txtDCport.Text = "25565";
			// 
			// txtDCip
			// 
			this.txtDCip.Location = new System.Drawing.Point(100, 45);
			this.txtDCip.Name = "txtDCip";
			this.txtDCip.Size = new System.Drawing.Size(130, 20);
			this.txtDCip.TabIndex = 17;
			this.txtDCip.Text = "127.0.0.1";
			// 
			// lblDCmppass
			// 
			this.lblDCmppass.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblDCmppass.Location = new System.Drawing.Point(10, 80);
			this.lblDCmppass.Name = "lblDCmppass";
			this.lblDCmppass.Size = new System.Drawing.Size(86, 20);
			this.lblDCmppass.TabIndex = 23;
			this.lblDCmppass.Text = "Mppass";
			// 
			// label2
			// 
			this.label2.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.label2.Location = new System.Drawing.Point(234, 45);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(10, 20);
			this.label2.TabIndex = 22;
			this.label2.Text = ":";
			// 
			// cbDCccskins
			// 
			this.cbDCccskins.Location = new System.Drawing.Point(10, 190);
			this.cbDCccskins.Name = "cbDCccskins";
			this.cbDCccskins.Size = new System.Drawing.Size(180, 20);
			this.cbDCccskins.TabIndex = 21;
			this.cbDCccskins.Text = "Use Classicube.net for skins";
			this.cbDCccskins.UseVisualStyleBackColor = true;
			// 
			// btnDCconnect
			// 
			this.btnDCconnect.AutoSize = true;
			this.btnDCconnect.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.btnDCconnect.Location = new System.Drawing.Point(10, 140);
			this.btnDCconnect.Name = "btnDCconnect";
			this.btnDCconnect.Size = new System.Drawing.Size(86, 30);
			this.btnDCconnect.TabIndex = 20;
			this.btnDCconnect.Text = "Connect";
			this.btnDCconnect.UseVisualStyleBackColor = true;
			this.btnDCconnect.Click += new System.EventHandler(this.BtnDCconnectClick);
			// 
			// lblDCaddress
			// 
			this.lblDCaddress.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblDCaddress.Location = new System.Drawing.Point(10, 45);
			this.lblDCaddress.Name = "lblDCaddress";
			this.lblDCaddress.Size = new System.Drawing.Size(86, 20);
			this.lblDCaddress.TabIndex = 16;
			this.lblDCaddress.Text = "Address";
			// 
			// lblDCuser
			// 
			this.lblDCuser.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.lblDCuser.Location = new System.Drawing.Point(10, 10);
			this.lblDCuser.Name = "lblDCuser";
			this.lblDCuser.Size = new System.Drawing.Size(120, 20);
			this.lblDCuser.TabIndex = 15;
			this.lblDCuser.Text = "Username";
			// 
			// tabs
			// 
			this.tabs.Controls.Add(this.tabDC);
			this.tabs.Controls.Add(this.tabClassicubeNet);
			this.tabs.Controls.Add(this.tabMinecraftNet);
			this.tabs.Location = new System.Drawing.Point(-1, 0);
			this.tabs.Name = "tabs";
			this.tabs.SelectedIndex = 0;
			this.tabs.Size = new System.Drawing.Size(482, 466);
			this.tabs.TabIndex = 0;
			// 
			// MainForm
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(484, 462);
			this.Controls.Add(this.tabs);
			this.Name = "MainForm";
			this.Text = "ClassicalSharp Launcher 0.95";
			this.ResizeEnd += new System.EventHandler(this.MainFormResizeEnd);
			this.tabClassicubeNet.ResumeLayout(false);
			this.tabCC.ResumeLayout(false);
			this.tabCCSignIn.ResumeLayout(false);
			this.tabCCSignIn.PerformLayout();
			this.tabCCServers.ResumeLayout(false);
			this.tabCCServers.PerformLayout();
			this.tabMinecraftNet.ResumeLayout(false);
			this.tabDC.ResumeLayout(false);
			this.tabDC.PerformLayout();
			this.tabs.ResumeLayout(false);
			this.ResumeLayout(false);
		}
		private System.Windows.Forms.Label lblMCdead;
		private System.Windows.Forms.Label lblMCdead2;
		private System.Windows.Forms.Label lblDChint;
		private System.Windows.Forms.Label lblDCuser;
		private System.Windows.Forms.Label lblDCaddress;
		private System.Windows.Forms.Button btnDCconnect;
		private System.Windows.Forms.CheckBox cbDCccskins;
		private System.Windows.Forms.Label label2;
		private System.Windows.Forms.Label lblDCmppass;
		private System.Windows.Forms.TextBox txtDCip;
		private System.Windows.Forms.TextBox txtDCport;
		private System.Windows.Forms.TextBox txtDCuser;
		private System.Windows.Forms.TextBox txtDCmppass;
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
		private System.Windows.Forms.TabControl tabs;
		private System.Windows.Forms.TabPage tabDC;
		private System.Windows.Forms.TabPage tabMinecraftNet;
	}
}