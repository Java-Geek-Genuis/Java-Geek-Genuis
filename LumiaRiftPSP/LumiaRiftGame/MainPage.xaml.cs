using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using Windows.Storage;
using Windows.Storage.AccessCache;
using Windows.Storage.Pickers;
using Windows.System;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Shapes;

namespace LumiaRiftGame
{
    public sealed partial class MainPage : Page
    {
        sealed class Star { public double X,Y,Speed; public Ellipse Visual; }
        sealed class Enemy { public double X,Y,VX,VY,Radius,Speed; public int HP,Kind; public Ellipse Visual; }
        sealed class Particle { public double X,Y,VX,VY,Life,MaxLife; public Ellipse Visual; }
        sealed class Shot { public double X,Y,VX,VY,Life; public Ellipse Visual; }

        readonly Random rng=new Random();
        readonly List<Star> stars=new List<Star>();
        readonly List<Enemy> enemies=new List<Enemy>();
        readonly List<Particle> particles=new List<Particle>();
        readonly List<Shot> shots=new List<Shot>();
        Polygon player;
        StorageFile selectedIso;
        DateTime lastFrame;
        double px,py,fireCooldown,spawnTimer,comboTimer,waveTimer;
        int score,combo,lives,wave;
        bool running,up,down,left,right,fire,boost;

        public MainPage(){InitializeComponent();Loaded+=LoadedPage;}

        void LoadedPage(object sender,RoutedEventArgs e)
        {
            MakeStars();
            lastFrame=DateTime.UtcNow;
            CompositionTarget.Rendering+=Frame;
            bool consent=ApplicationData.Current.LocalSettings.Values.ContainsKey("PspIsoConsent") &&
                         (bool)ApplicationData.Current.LocalSettings.Values["PspIsoConsent"];
            ConsentStatus.Text=consent?"ISO picker consent is enabled.":"The first ISO selection asks for permission.";
        }

        void MakeStars()
        {
            if(stars.Count>0)return;
            for(int i=0;i<66;i++)
            {
                double size=1+rng.NextDouble()*3.2;
                Ellipse e=new Ellipse{Width=size,Height=size,
                    Fill=new SolidColorBrush(Color.FromArgb((byte)(80+rng.Next(120)),170,220,255))};
                GameCanvas.Children.Add(e);
                stars.Add(new Star{X=rng.NextDouble()*1000,Y=rng.NextDouble()*700,
                    Speed=15+rng.NextDouble()*60,Visual=e});
            }
        }

        void Frame(object sender,object e)
        {
            if(!running){lastFrame=DateTime.UtcNow;return;}
            double dt=(DateTime.UtcNow-lastFrame).TotalSeconds;
            lastFrame=DateTime.UtcNow;
            if(dt>0.08)dt=0.08;
            UpdateGame(dt);
        }

        void StartGame()
        {
            HidePanels(); HudGrid.Visibility=Visibility.Visible;
            TouchOverlay.Visibility=Visibility.Visible; TouchOverlay.IsHitTestVisible=true;
            ClearObjects();
            score=0;combo=1;lives=3;wave=1;fireCooldown=0;spawnTimer=.25;comboTimer=0;waveTimer=0;
            px=Math.Max(240,GameCanvas.ActualWidth*.5);
            py=Math.Max(300,GameCanvas.ActualHeight*.72);
            player=new Polygon
            {
                Points=new Windows.Foundation.Collections.PointCollection{
                    new Windows.Foundation.Point(0,-25),new Windows.Foundation.Point(17,22),
                    new Windows.Foundation.Point(0,12),new Windows.Foundation.Point(-17,22)},
                Fill=new SolidColorBrush(Color.FromArgb(255,97,230,255)),
                Stroke=new SolidColorBrush(Color.FromArgb(255,240,255,255)),StrokeThickness=2
            };
            GameCanvas.Children.Add(player);
            Canvas.SetLeft(player,px);Canvas.SetTop(player,py);
            running=true;UpdateHud();
        }

        void HidePanels(){MenuPanel.Visibility=Visibility.Collapsed;PspPanel.Visibility=Visibility.Collapsed;
            ConsentPanel.Visibility=Visibility.Collapsed;GameOverPanel.Visibility=Visibility.Collapsed;SettingsPanel.Visibility=Visibility.Collapsed;}

        void ClearObjects()
        {
            foreach(Enemy e in enemies)GameCanvas.Children.Remove(e.Visual);
            foreach(Particle p in particles)GameCanvas.Children.Remove(p.Visual);
            foreach(Shot s in shots)GameCanvas.Children.Remove(s.Visual);
            enemies.Clear();particles.Clear();shots.Clear();
            if(player!=null){GameCanvas.Children.Remove(player);player=null;}
        }

        void UpdateGame(double dt)
        {
            double w=Math.Max(480,GameCanvas.ActualWidth),h=Math.Max(700,GameCanvas.ActualHeight);
            foreach(Star s in stars)
            {
                s.Y+=s.Speed*dt*(boost?2.15:1);
                if(s.Y>h+10){s.Y=-8;s.X=rng.NextDouble()*w;}
                Canvas.SetLeft(s.Visual,s.X);Canvas.SetTop(s.Visual,s.Y);
            }

            double speed=boost?370:230;
            if(left)px-=speed*dt;if(right)px+=speed*dt;if(up)py-=speed*dt;if(down)py+=speed*dt;
            px=Clamp(px,28,w-28);py=Clamp(py,95,h-50);
            Canvas.SetLeft(player,px);Canvas.SetTop(player,py);

            fireCooldown-=dt;
            if(fire&&fireCooldown<=0){Shoot();fireCooldown=boost?.1:.18;}
            spawnTimer-=dt;
            if(spawnTimer<=0){SpawnEnemy();spawnTimer=Math.Max(.18,.66-wave*.035);}

            for(int i=enemies.Count-1;i>=0;i--)
            {
                Enemy en=enemies[i];
                double dx=px-en.X,dy=py-en.Y,len=Math.Sqrt(dx*dx+dy*dy);
                if(len>.001){dx/=len;dy/=len;}
                if(en.Kind==0){en.VX+=dx*en.Speed*dt;en.VY+=dy*en.Speed*dt;}
                else {en.VX+=(-dy*en.Speed*.7+dx*en.Speed*.35)*dt;en.VY+=(dx*en.Speed*.7+dy*en.Speed*.35)*dt;}
                double es=Math.Sqrt(en.VX*en.VX+en.VY*en.VY),max=en.Kind==0?145:120;
                if(es>max){en.VX=en.VX/es*max;en.VY=en.VY/es*max;}
                en.X+=en.VX*dt;en.Y+=en.VY*dt;
                Canvas.SetLeft(en.Visual,en.X-en.Radius);Canvas.SetTop(en.Visual,en.Y-en.Radius);
                if(Distance(en.X,en.Y,px,py)<en.Radius+20)
                {
                    Burst(en.X,en.Y,16,160);GameCanvas.Children.Remove(en.Visual);enemies.RemoveAt(i);
                    lives--;combo=1;comboTimer=0;if(lives<=0){EndGame();return;}
                }
                else if(en.Y>h+80||en.X<-100||en.X>w+100)
                {GameCanvas.Children.Remove(en.Visual);enemies.RemoveAt(i);}
            }

            for(int i=shots.Count-1;i>=0;i--)
            {
                Shot s=shots[i];s.X+=s.VX*dt;s.Y+=s.VY*dt;s.Life-=dt;
                Canvas.SetLeft(s.Visual,s.X-4);Canvas.SetTop(s.Visual,s.Y-9);
                bool remove=s.Life<=0||s.X<-30||s.X>w+30||s.Y<-30;
                if(!remove)
                for(int j=enemies.Count-1;j>=0;j--)
                {
                    Enemy en=enemies[j];
                    if(Distance(s.X,s.Y,en.X,en.Y)<en.Radius+9)
                    {
                        en.HP--;remove=true;
                        if(en.HP<=0)
                        {
                            score+=40+combo*10;combo=Math.Min(99,combo+1);comboTimer=2.7;
                            Burst(en.X,en.Y,12,220);GameCanvas.Children.Remove(en.Visual);enemies.RemoveAt(j);
                        }
                        break;
                    }
                }
                if(remove){GameCanvas.Children.Remove(s.Visual);shots.RemoveAt(i);}
            }

            for(int i=particles.Count-1;i>=0;i--)
            {
                Particle p=particles[i];p.X+=p.VX*dt;p.Y+=p.VY*dt;p.Life-=dt;p.VX*=.97;p.VY*=.97;
                Canvas.SetLeft(p.Visual,p.X);Canvas.SetTop(p.Visual,p.Y);p.Visual.Opacity=Math.Max(0,p.Life/p.MaxLife);
                if(p.Life<=0){GameCanvas.Children.Remove(p.Visual);particles.RemoveAt(i);}
            }

            comboTimer-=dt;if(comboTimer<=0&&combo>1)combo--;
            waveTimer+=dt;
            if(waveTimer>24+wave*2){wave++;waveTimer=0;score+=250;Burst(w*.5,140,20,120);}
            UpdateHud();
        }

        void SpawnEnemy()
        {
            double w=Math.Max(480,GameCanvas.ActualWidth);int kind=rng.Next(3)==0?1:0;double r=kind==0?18:23;
            double x=45+rng.NextDouble()*Math.Max(1,w-90);
            Ellipse v=new Ellipse{Width=r*2,Height=r*2,
                Fill=new SolidColorBrush(kind==0?Color.FromArgb(255,255,94,152):Color.FromArgb(255,255,184,83)),
                Stroke=new SolidColorBrush(Color.FromArgb(255,255,240,255)),StrokeThickness=1.5};
            GameCanvas.Children.Add(v);
            enemies.Add(new Enemy{X=x,Y=-55,Radius=r,Kind=kind,Speed=kind==0?40+wave*4:31+wave*3,
                VX=(rng.NextDouble()-.5)*30,VY=35+rng.NextDouble()*40,Visual=v,HP=kind==0?1:2});
        }

        void Shoot(){Ellipse v=new Ellipse{Width=8,Height=18,Fill=new SolidColorBrush(Colors.White)};
            GameCanvas.Children.Add(v);shots.Add(new Shot{X=px,Y=py-24,VY=-720,Life=1.1,Visual=v});}

        void Burst(double x,double y,int count,double spread)
        {
            for(int i=0;i<count;i++)
            {
                double a=rng.NextDouble()*Math.PI*2,speed=35+rng.NextDouble()*spread,life=.5+rng.NextDouble()*.65;
                Ellipse v=new Ellipse{Width=4+rng.NextDouble()*5,Height=4+rng.NextDouble()*5,
                    Fill=new SolidColorBrush(Color.FromArgb(255,255,210,113))};
                GameCanvas.Children.Add(v);
                particles.Add(new Particle{X=x,Y=y,VX=Math.Cos(a)*speed,VY=Math.Sin(a)*speed,Life=life,MaxLife=life,Visual=v});
            }
        }

        void UpdateHud(){ScoreText.Text="SCORE "+score.ToString("000000");ComboText.Text="   COMBO x"+combo;
            WaveText.Text="WAVE "+wave;LivesText.Text="   "+new string('♥',Math.Max(0,lives));}

        void EndGame(){running=false;TouchOverlay.IsHitTestVisible=false;GameOverPanel.Visibility=Visibility.Visible;
            FinalScoreText.Text="SCORE "+score.ToString("000000")+"   WAVE "+wave;}

        void PlayButton_Click(object s,RoutedEventArgs e){StartGame();}
        void RestartButton_Click(object s,RoutedEventArgs e){StartGame();}
        void MenuButton_Click(object s,RoutedEventArgs e){running=false;ClearObjects();HudGrid.Visibility=Visibility.Collapsed;
            TouchOverlay.Visibility=Visibility.Collapsed;HidePanels();MenuPanel.Visibility=Visibility.Visible;}

        void SettingsButton_Click(object s,RoutedEventArgs e){MenuPanel.Visibility=Visibility.Collapsed;SettingsPanel.Visibility=Visibility.Visible;}
        void SettingsBackButton_Click(object s,RoutedEventArgs e){SettingsPanel.Visibility=Visibility.Collapsed;MenuPanel.Visibility=Visibility.Visible;}
        void PspButton_Click(object s,RoutedEventArgs e){MenuPanel.Visibility=Visibility.Collapsed;PspPanel.Visibility=Visibility.Visible;}
        void BackFromPspButton_Click(object s,RoutedEventArgs e){PspPanel.Visibility=Visibility.Collapsed;MenuPanel.Visibility=Visibility.Visible;}
        void ResetConsentButton_Click(object s,RoutedEventArgs e){ApplicationData.Current.LocalSettings.Values.Remove("PspIsoConsent");ConsentStatus.Text="Consent reset. The next ISO selection will ask again.";}

        async void ChooseIsoButton_Click(object s,RoutedEventArgs e)
        {
            bool consent=ApplicationData.Current.LocalSettings.Values.ContainsKey("PspIsoConsent") &&
                         (bool)ApplicationData.Current.LocalSettings.Values["PspIsoConsent"];
            if(!consent){ConsentPanel.Visibility=Visibility.Visible;return;}
            await PickIsoAsync();
        }

        async void ConsentAllowButton_Click(object s,RoutedEventArgs e)
        {
            ApplicationData.Current.LocalSettings.Values["PspIsoConsent"]=true;
            ConsentPanel.Visibility=Visibility.Collapsed;
            ConsentStatus.Text="Thanks. Windows will now ask you to choose the ISO.";
            await PickIsoAsync();
        }

        void ConsentCancelButton_Click(object s,RoutedEventArgs e){ConsentPanel.Visibility=Visibility.Collapsed;ConsentStatus.Text="No file was opened.";}

        async Task PickIsoAsync()
        {
            try
            {
                FileOpenPicker p=new FileOpenPicker{ViewMode=PickerViewMode.Thumbnail,SuggestedStartLocation=PickerLocationId.DocumentsLibrary};
                p.FileTypeFilter.Add(".iso");
                StorageFile f=await p.PickSingleFileAsync();
                if(f==null){IsoStatus.Text="No ISO selected.";LaunchIsoButton.IsEnabled=false;return;}
                selectedIso=f;
                try{StorageApplicationPermissions.FutureAccessList.AddOrReplace("LastPspIso",f);}catch{}
                BasicProperties props=await f.GetBasicPropertiesAsync();
                IsoStatus.Text=f.Name+"\\n"+(props.Size/1024.0/1024.0).ToString("0.0")+" MB";
                LaunchIsoButton.IsEnabled=true;
            }
            catch(Exception ex){IsoStatus.Text="Could not open the picker: "+ex.Message;}
        }

        async void LaunchIsoButton_Click(object s,RoutedEventArgs e)
        {
            if(selectedIso==null)return;
            try
            {
                bool ok=await Launcher.LaunchFileAsync(selectedIso);
                IsoStatus.Text=ok?selectedIso.Name+"\\nLaunch request sent to the registered PSP/ISO app."
                    :selectedIso.Name+"\\nNo registered ISO app accepted the file. Build the PPSSPP ARM32 path from ThirdParty.";
            }
            catch(Exception ex){IsoStatus.Text="Launch failed: "+ex.Message;}
        }

        void Control_PointerPressed(object s,PointerRoutedEventArgs e){Button b=s as Button;if(b==null)return;SetControl((string)b.Tag,true);}
        void Control_PointerReleased(object s,PointerRoutedEventArgs e){Button b=s as Button;if(b==null)return;SetControl((string)b.Tag,false);}
        void SetControl(string tag,bool v){if(tag=="Up")up=v;else if(tag=="Down")down=v;else if(tag=="Left")left=v;else if(tag=="Right")right=v;else if(tag=="Fire")fire=v;else if(tag=="Boost")boost=v;}

        void Page_KeyDown(object s,KeyRoutedEventArgs e)
        {
            switch(e.Key)
            {
                case VirtualKey.Up:case VirtualKey.W:up=true;break;case VirtualKey.Down:case VirtualKey.S:down=true;break;
                case VirtualKey.Left:case VirtualKey.A:left=true;break;case VirtualKey.Right:case VirtualKey.D:right=true;break;
                case VirtualKey.Z:fire=true;break;case VirtualKey.Shift:boost=true;break;
            }
        }
        void Page_KeyUp(object s,KeyRoutedEventArgs e)
        {
            switch(e.Key)
            {
                case VirtualKey.Up:case VirtualKey.W:up=false;break;case VirtualKey.Down:case VirtualKey.S:down=false;break;
                case VirtualKey.Left:case VirtualKey.A:left=false;break;case VirtualKey.Right:case VirtualKey.D:right=false;break;
                case VirtualKey.Z:fire=false;break;case VirtualKey.Shift:boost=false;break;
            }
        }

        static double Clamp(double n,double lo,double hi){return Math.Max(lo,Math.Min(hi,n));}
        static double Distance(double ax,double ay,double bx,double by){double x=ax-bx,y=ay-by;return Math.Sqrt(x*x+y*y);}
    }
}
