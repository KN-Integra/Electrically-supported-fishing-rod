using System.Buffers.Text;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using DriverHostapp.Backend.Utils.ErrorCodes;
using DriverHostapp.Frontend.HostappMainWindow;

namespace DriverHostapp.Frontend.GeneralAppSettings;

public partial class GeneralSettings : Window{

    MainWindow ParentWindow;
    public GeneralSettings(MainWindow parentWindow){
        ParentWindow = parentWindow;
        InitializeComponent();

        TextBlock? front_version_text = this.Find<TextBlock>("Front_version");
        if(front_version_text is not null){
            front_version_text.Text = $"Frontend Version {ParentWindow.FrontendVersion.MajorVersion}.{ParentWindow.FrontendVersion.MinorVersion}";
        }

        TextBlock? serial_version_text = this.Find<TextBlock>("Serial_version");
        if(serial_version_text is not null){
            serial_version_text.Text = $"Serial Version {ParentWindow.BackendImplementations[0].GetVersion().MajorVersion}.{ParentWindow.BackendImplementations[0].GetVersion().MinorVersion}";
        }

        TextBlock? bt_version_text = this.Find<TextBlock>("Bluetooth_version");
        if(bt_version_text is not null){
            bt_version_text.Text = $"BT Version {ParentWindow.BackendImplementations[1].GetVersion().MajorVersion}.{ParentWindow.BackendImplementations[1].GetVersion().MinorVersion}";
        }
    }

    public new void Show(){
        base.Show();
        CheckBox? loggingCheckBox = this.Find<CheckBox>("LoggingCheckbox");
        if(loggingCheckBox is not null){
            loggingCheckBox.IsChecked = (ParentWindow.Logger is not null);
        }
    }

    private void LoggingCheckBox_Checked(object sender, RoutedEventArgs e){
        if(ParentWindow.Logger is null){
            ParentWindow.EnableLogging();
        }
    }

    private void LoggingCheckBox_UnChecked(object sender, RoutedEventArgs e){
        if(ParentWindow.Logger is not null){
            ParentWindow.DisableLogging();
        }
    }
}
