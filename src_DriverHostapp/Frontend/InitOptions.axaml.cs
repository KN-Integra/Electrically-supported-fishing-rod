using Avalonia.Controls;
using Avalonia.Interactivity;

// using Windows.Storage;

namespace DriverHostapp.Frontend.HostappInitOptions;

public partial class InitOptions : Window{
    public InitOptions(){
        InitializeComponent();
    }

    public void UpdateInitValues(object sender, RoutedEventArgs e){

    }

    [System.Obsolete]
    public async void SaveInitConfigToFile(object sender, RoutedEventArgs e){
        SaveFileDialog saveFileDialog = new SaveFileDialog();
        saveFileDialog.DefaultExtension = ".json";
        await saveFileDialog.ShowAsync(this);
    }

    public void LoadInitConfigToFile(object sender, RoutedEventArgs e){

    }
}
