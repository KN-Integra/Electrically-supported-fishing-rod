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
    }

    public new void Show(){
        base.Show();
        CheckBox? loggingCheckBox = this.Find<CheckBox>("LoggingCheckbox");
        if(loggingCheckBox is not null){
            loggingCheckBox.IsChecked = (ParentWindow.Logger is not null);
        }
    }

    public void LoggingCheckBox_Checked(object sender, RoutedEventArgs e){
        if(ParentWindow.Logger is null){
            ParentWindow.EnableLogging();
        }
    }

    public void LoggingCheckBox_UnChecked(object sender, RoutedEventArgs e){
        if(ParentWindow.Logger is not null){
            ParentWindow.DisableLogging();
        }
    }
}
