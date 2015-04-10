using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;
using System.Diagnostics;
using SoftwareRasterizer.Engine;
using OpenTK;

namespace SoftwareRasterizer {

	public partial class MainForm : Form {
		
		SoftwareEngine device;
		public MainForm() {
			InitializeComponent();
			ClientSize = new Size( 640, 480 );
			Application.Idle += Application_Idle;
			SetStyle( ControlStyles.AllPaintingInWmPaint | ControlStyles.UserPaint, true );
			//SetStyle( ControlStyles.OptimizedDoubleBuffer, true );
			SetStyle( ControlStyles.DoubleBuffer, true );
			device = new SoftwareEngine( 640, 480 );
		}
		
		void Application_Idle( object sender, EventArgs e ) {
			Invalidate();
		}
		
		Vector3 rot = Vector3.Zero;
		int frames = 0;
		protected override void OnPaint( PaintEventArgs e ) {
			try {
				var sw = System.Diagnostics.Stopwatch.StartNew();
				device.Clear( new FastColour( 0, 0, 0, 255 ) );

				Matrix4 view = Matrix4.LookAt( new Vector3( 0, 0, 10 ), Vector3.Zero, Vector3.UnitY );
				//Matrix4 view = Matrix4.LookAt( new Vector3( 10 * (float)Math.Sin( rot.X * 1 ), 0, 10 * (float)Math.Cos( rot.X * 1 ) ), Vector3.Zero, Vector3.UnitY );
				Matrix4 proj = Matrix4.Perspective( 0.78f, 640f / 480f, 0.01f, 50.0f );
				
				Vector3[] vertices = new Vector3[8];
				vertices[0] = new Vector3(-1, 1, 1);
				vertices[1] = new Vector3(1, 1, 1);
				vertices[2] = new Vector3(-1, -1, 1);
				vertices[3] = new Vector3(1, -1, 1);
				vertices[4] = new Vector3(-1, -1, -1);
				vertices[5] = new Vector3(-1, 1, -1);
				vertices[6] = new Vector3(1, 1, -1);
				vertices[7] = new Vector3(1, -1, -1);
				ushort[] indices = {
					// Z+
					0, 1, 2,
					1, 2, 3,
					// Z-
					4, 5, 6,
					5, 6, 7,
					// X+
					1, 3, 6,
					3, 6, 7,
					// X-
					0, 2, 4,
					2, 4, 5,
					// Y+
					0, 1, 5,
					1, 5, 6,
					// Y-
					2, 3, 4,
					2, 4, 7,		
				};
				
				rot.X += 0.01f;
				Matrix4 world = Matrix4.RotateX( rot.X ) * Matrix4.RotateY( rot.X );
				device.SetAllMatrices( ref proj, ref view, ref world );
				device.Filled_DrawIndexedTriangles( vertices, indices );
				/*for( int i = 0; i < 1000; i++ ) {
					float offset = (float)rnd.NextDouble() - 0.5f;
					for( int j = 0; j < vertices.Length; j++ ) {
						vertices[j] += new Vector3( offset, offset, offset );
					}
					device.Line_DrawIndexedTriangles( vertices, indices );
				}*/
				
				device.Present( e.Graphics );
				base.OnPaint( e );
				frames++;
				sw.Stop();
				this.Text = sw.ElapsedMilliseconds.ToString();
			} catch( Exception ex ) {
				System.Diagnostics.Debug.WriteLine( ex );
				System.Diagnostics.Debugger.Break();
				throw;
			}
		}
		
		Random rnd = new Random();
		
		protected override void OnResize( EventArgs e ) {
			if( device != null ) {
				device.Resize( ClientSize.Width, ClientSize.Height );
			}
		}
	}
}
