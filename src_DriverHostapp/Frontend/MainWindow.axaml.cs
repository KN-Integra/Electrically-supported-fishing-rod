using Avalonia.Controls;
using Avalonia.Interactivity;

using System;
using System.Collections.Generic;

using DriverHostapp.Backend.CallbackInterface;
using DriverHostapp.Backend.Utils.ErrorCodes;
using DriverHostapp.Backend.CallbackSerialShell;
using DriverHostapp.Backend.CallbackBT;
using DriverHostapp.Backend.Utils.ControlModes;

using DriverHostapp.Frontend.HostappMessageBox;
using DriverHostapp.Frontend.HostappSerialConnectionOptions;
using DriverHostapp.Frontend.HostappInitOptions;
using DriverHostapp.Frontend.GeneralAppSettings;
using DriverHostapp.Backend.Utils.SoftwareVersion;

namespace DriverHostapp.Frontend.HostappMainWindow{
    public partial class MainWindow : Window{
        private uint? TechnologyIndex;
        public List<IHostappBackend> BackendImplementations {get; }

        public NLog.Logger? Logger = null;

        public SoftwareVersion FrontendVersion = new(){
            MajorVersion = 1,
            MinorVersion = 1
        };

        public MainWindow(){
            BackendImplementations = new List<IHostappBackend>
            {
                new HostappBackendSerial(),
                new HostappBackendBluetooth()
            };

            TechnologyIndex = null;

            //this.EnableLogging(); // Enable in emergency!

            // Generated with Avalonia.NameGenerator
            InitializeComponent();
        }

        private void OpenAppSettings(object sender, RoutedEventArgs args){
            try{
                GeneralSettings window = new GeneralSettings(this);
                window.Show();
            } catch(Exception ex){
                this.openMessageWindow(ex.Message);
                return;
            }
        }

        public void EnableLogging(){
            Logger = NLog.LogManager.GetCurrentClassLogger();

            if(this.TechnologyIndex is null){
                return;
            }

            this.BackendImplementations[(int)this.TechnologyIndex].SetLogger(this.Logger);

            Logger.Info("Logger Starting!");
        }

        public void DisableLogging(){
            Logger?.Info("Logger Stoping!");
            Logger = null;
        }

        private void ConnType_SelectionChanged(object sender, SelectionChangedEventArgs e){
            if(this.TechnologyIndex is not null){
                try{
                    this.BackendImplementations[(int)this.TechnologyIndex].CloseConnection();
                } catch(DeviceNotConnected){
                    //TODO logging
                } catch(Exception){
                    // TODO - logging
                }
            }

            ComboBox comboBox = (ComboBox)sender;

            Logger?.Info($"Selected Technology {comboBox.SelectedIndex}");

            if(comboBox.SelectedIndex > 0){
                TechnologyIndex = (uint)comboBox.SelectedIndex-1;
            }
            else {
                TechnologyIndex = null;
            }

            if(this.TechnologyIndex is not null){
                this.BackendImplementations[(int)this.TechnologyIndex].SetLogger(this.Logger);
            }

            Button? button = this.Find<Button>("ScanDevicesButton");
            StackPanel? panel = this.Find<StackPanel>("ScanDevicesPanel");
            if(button is null || panel is null){
                this.openMessageWindow($"{button} or {panel} doesn't exist!");
                return;
            }

            panel.IsEnabled = (comboBox.SelectedIndex != 0);

            ComboBoxItem? singleItem = (ComboBoxItem?)comboBox.SelectedItem;
            if(singleItem is null){
                this.openMessageWindow("Chosen technology is null");
                return;
            }

            if(panel.IsEnabled){
                button.Content = $"Scan {singleItem.Content} devices";
            } else{
                button.Content = $"Scan devices";
                ComboBox? comboBox_devSelection = this.Find<ComboBox>("SelectDeviceComboBox");
                StackPanel? panel_devSelection = this.Find<StackPanel>("SelectDevicePanel");
                if(comboBox_devSelection is null || panel_devSelection is null){
                    this.openMessageWindow($"{comboBox_devSelection} or {panel_devSelection} doesn't exist!");
                    return;
                }

                panel_devSelection.IsEnabled = false;
                comboBox_devSelection.Items.Clear();
                comboBox_devSelection.SelectedIndex = 0;

                this.disableStackPanels(new List<string>{
                    "ConnectToDevicePanel",
                    "SendInitPanel",
                    "SetModePanel",
                    "GetModePanel",
                    "SetSpeedPanel",
                    "GetSpeedPanel",
                    "SetPosPanel",
                    "GetPosPanel",
                    "OffOnPanel"
                });
            }
        }

        private void ScanDevices(object sender, RoutedEventArgs args){
            if(this.TechnologyIndex is null){
                this.openMessageWindow("Technology is null!");
                return;
            }
            ComboBox? comboBox = this.Find<ComboBox>("SelectDeviceComboBox");
            StackPanel? panel = this.Find<StackPanel>("SelectDevicePanel");
            if(comboBox is null || panel is null){
                this.openMessageWindow($"{comboBox} or {panel} doesn't exist!");
                return;
            }

            comboBox.Items.Clear();
            panel.IsEnabled = true;
            comboBox.Items.Add("Select Device");
            try{
                this.BackendImplementations[(int)this.TechnologyIndex].ListDevices();
                List<string> connected_device_list = this.BackendImplementations[(int)this.TechnologyIndex].GetConnectionsAsListOfStrings();
                foreach(string s in connected_device_list){
                    comboBox.Items.Add(s);
                }
            } catch(Exception ex){
                Logger?.Info($"Selected Technology {ex.Message}");
            }
            comboBox.SelectedIndex = 0;
        }



        private void SelectDevice_SelectionChanged(object sender, SelectionChangedEventArgs e){
            if(this.TechnologyIndex is null){
                return;
            }

            try{
                this.BackendImplementations[(int)this.TechnologyIndex].CloseConnection();
            } catch(DeviceNotConnected){
                //TODO logging
            } catch(Exception){
                // TODO - logging
            }

            ComboBox comboBox = (ComboBox)sender;

            StackPanel? connect_to_dev_panel = this.Find<StackPanel>("ConnectToDevicePanel");
            if(connect_to_dev_panel is null){
                this.openMessageWindow($"{connect_to_dev_panel} doesn't exist!");
                return;
            }

            connect_to_dev_panel.IsEnabled = (comboBox.SelectedIndex != 0);
            if(comboBox.SelectedIndex > 0){
                try{
                    this.BackendImplementations[(int)this.TechnologyIndex].ChooseConnectionByIndex((uint)comboBox.SelectedIndex-1);
                } catch(Exception ex){
                    this.openMessageWindow(ex.Message);
                    return;
                }
            }

            if(comboBox.SelectedIndex == 0){
                try{
                    this.BackendImplementations[(int)this.TechnologyIndex].CloseConnection();
                } catch(Exception) {

                }

            }

            this.disableStackPanels(new List<string>{
                "SendInitPanel",
                "SetModePanel",
                "GetModePanel",
                "SetSpeedPanel",
                "GetSpeedPanel",
                "SetPosPanel",
                "GetPosPanel",
                "OffOnPanel"
            });
        }

        private void ConnectDevice(object sender, RoutedEventArgs args){
            if(this.TechnologyIndex is null){
                this.openMessageWindow("Technology is null!");
                return;
            }

            StackPanel? button = this.Find<StackPanel>("SendInitPanel");
            if(button is null){
                this.openMessageWindow($"{button} doesn't exist!");
                return;
            }

            button.IsEnabled = true;

            try{
                this.BackendImplementations[(int)this.TechnologyIndex].OpenConnection();
            } catch(Exception ex){
                this.openMessageWindow(ex.Message);
                return;
            }
        }

        private void SetConnectionOptions(object sender, RoutedEventArgs args){
            var window = new ConnectionOptions();
            window.Show();
        }


        private void Init(object sender, RoutedEventArgs args){
            if(this.TechnologyIndex is null){
                this.openMessageWindow("Technology is null!");
                return;
            }

            uint default_speed = 0;
            uint default_position = 0;
            try{
                this.BackendImplementations[(int)this.TechnologyIndex].SendConfiguration();
                default_speed = this.BackendImplementations[(int)this.TechnologyIndex].GetSpeed();
                default_position = this.BackendImplementations[(int)this.TechnologyIndex].GetPosition();
            }  catch(Exception ex){
                this.openMessageWindow(ex.Message);
                return;
            }

            this.updateModeTextBox();

            TextBox? get_speed_textbox = this.Find<TextBox>("GetSpeedText");
            TextBox? get_pos_textbox = this.Find<TextBox>("GetPosText");

            if(get_speed_textbox is null || get_pos_textbox is null){
                this.openMessageWindow($"{get_speed_textbox} or {get_pos_textbox} doesn't exist!");
                return;
            }

            get_speed_textbox.Text = default_speed.ToString();
            get_pos_textbox.Text = default_position.ToString();

            List<string> panels_to_enable = new List<string>{
                "SetModePanel",
                "GetModePanel",
                "GetSpeedPanel",
                "GetPosPanel",
                "OffOnPanel"
            };

            foreach (string panel_name in panels_to_enable){
                StackPanel? enabled_functionality = this.Find<StackPanel>(panel_name);

                if(enabled_functionality is null){
                    this.openMessageWindow($"{enabled_functionality} doesn't exist!");
                    return;
                }

                enabled_functionality.IsEnabled = true;
            }


            this.enableDisableGuiElementsOffOn();
            this.enableDisableGuiElementsSetSpeedPos();
        }

        private void SetInitParams(object sender, RoutedEventArgs args){
            var window = new InitOptions();
            window.Show();
        }

        private void GetMode(object sender, RoutedEventArgs args){
            updateModeTextBox();
        }

        private void updateModeTextBox(){
            if(this.TechnologyIndex is null){
                this.openMessageWindow("Technology is null!");
                return;
            }

            TextBox? get_mode_textbox = this.Find<TextBox>("GetModeText");

            if(get_mode_textbox is null){
                this.openMessageWindow($"{get_mode_textbox} doesn't exist!");
                return;
            }

            ControlMode recievedControlMode = ControlMode.Speed;

            try{
                recievedControlMode = this.BackendImplementations[(int)this.TechnologyIndex].GetMode();
            } catch(Exception ex){
                this.openMessageWindow(ex.Message);
                return;
            }
            get_mode_textbox.Text = recievedControlMode.ToString();
        }

        private void SetMode(object sender, RoutedEventArgs args){
            ComboBox? set_mode_combo = this.Find<ComboBox>("SendModeComboBox");

            if(set_mode_combo is null){
                this.openMessageWindow($"{set_mode_combo} doesn't exist!");
                return;
            }

            ComboBoxItem? single_item = (ComboBoxItem?)set_mode_combo.Items[set_mode_combo.SelectedIndex];
            if(single_item is null){
                this.openMessageWindow($"Single mode combo box item from under index {set_mode_combo.SelectedIndex} doesn't exist!");
                return;
            }

            if(single_item.Content is null){
                this.openMessageWindow("Selected ComboBox item doesn't have conent!");
                return;
            }

            ControlMode value_to_set;
            if (!Enum.TryParse(single_item.Content.ToString(), out value_to_set)){
                this.openMessageWindow("Value from mode ComboBox could not be parsed into Enum!");
                return;
            }

            if(this.TechnologyIndex is null){
                this.openMessageWindow("Technology is null!");
                return;
            }

            ControlMode value_returned = ControlMode.Speed;

            try{
                this.BackendImplementations[(int)this.TechnologyIndex].SetMode(value_to_set);
                value_returned = this.BackendImplementations[(int)this.TechnologyIndex].GetMode();
            } catch(Exception ex){
                this.openMessageWindow(ex.Message);
                return;
            }

            if(value_returned != value_to_set){
                this.openMessageWindow("Value couldn't be set - value was not changed on the device!");
            } else {
                this.updateModeTextBox();
                this.enableDisableGuiElementsSetSpeedPos();
            }
        }

        private void GetSpeed(object sender, RoutedEventArgs args){
            if(this.TechnologyIndex is null){
                this.openMessageWindow("Technology is null!");
                return;
            }

            uint default_speed = 0;
            try{
                default_speed = this.BackendImplementations[(int)this.TechnologyIndex].GetSpeed();
            } catch(Exception ex){
                this.openMessageWindow(ex.Message);
                return;
            }

            TextBox? get_speed_textbox = this.Find<TextBox>("GetSpeedText");
            if(get_speed_textbox is null){
                this.openMessageWindow($"{get_speed_textbox} doesn't exist!");
                return;
            }

            get_speed_textbox.Text = default_speed.ToString();
        }

        private void SetSpeed(object sender, RoutedEventArgs args){
            if(this.TechnologyIndex is null){
                this.openMessageWindow("Technology is null!");
                return;
            }

            uint speed_to_set = 0u;
            TextBox? set_speed_textbox = this.Find<TextBox>("SetSpeedText");
            if(set_speed_textbox is null){
                this.openMessageWindow($"{set_speed_textbox} doesn't exist!");
                return;
            }

            if(!UInt32.TryParse(set_speed_textbox.Text, out speed_to_set)){
                this.openMessageWindow("Parsing error, non numeric value in text box!");
                return;
            }

            try{
                this.BackendImplementations[(int)this.TechnologyIndex].SetSpeed(speed_to_set);
            } catch(Exception ex){
                this.openMessageWindow(ex.Message);
                return;
            }
        }

        private void GetPosition(object sender, RoutedEventArgs args){
            if(this.TechnologyIndex is null){
                this.openMessageWindow("Technology is null!");
                return;
            }
            uint position = this.BackendImplementations[(int)this.TechnologyIndex].GetPosition();

            TextBox? get_pos_textbox = this.Find<TextBox>("GetPosText");

            if(get_pos_textbox is null){
                this.openMessageWindow($"{get_pos_textbox} doesn't exist!");
                return;
            }

            get_pos_textbox.Text = position.ToString();
        }

        private void SetPosition(object sender, RoutedEventArgs args){
            if(this.TechnologyIndex is null){
                this.openMessageWindow("Technology is null!");
                return;
            }
            TextBox? set_pos_textbox = this.Find<TextBox>("SetPosText");

            if(set_pos_textbox is null){
                this.openMessageWindow($"{set_pos_textbox} doesn't exist!");
                return;
            }

            uint pos_to_set;
            if(!UInt32.TryParse(set_pos_textbox.Text, out pos_to_set)){
                this.openMessageWindow("Parsing error, non numeric value in text box!");
                return;
            }
            this.BackendImplementations[(int)this.TechnologyIndex].SetPosition(pos_to_set);
        }

        private void OffOnDevice(object sender, RoutedEventArgs args){
            if(this.TechnologyIndex is null){
                this.openMessageWindow("Technology is null!");
                return;
            }

            try{
                bool default_off_on_state = this.BackendImplementations[(int)this.TechnologyIndex].GetOffOnState();
                if(default_off_on_state){
                    this.BackendImplementations[(int)this.TechnologyIndex].TurnDriverOff();
                } else {
                    this.BackendImplementations[(int)this.TechnologyIndex].TurnDriverOn();
                }
            } catch(Exception ex){
                this.openMessageWindow(ex.Message);
                return;
            }

            this.enableDisableGuiElementsOffOn();
        }

        private void GetOffOnState(object sender, RoutedEventArgs args){
            this.enableDisableGuiElementsOffOn();
        }

        private void enableDisableGuiElementsOffOn(){
            if(this.TechnologyIndex is null){
                this.openMessageWindow("Technology is null!");
                return;
            }
            Button? turn_off_on_button = this.Find<Button>("OffOnButton");
            TextBox? get_on_off_state = this.Find<TextBox>("OffOnText");
            if(turn_off_on_button is null || get_on_off_state is null){
                this.openMessageWindow($"{turn_off_on_button} or {get_on_off_state} doesn't exist!");
                return;
            }

            bool default_off_on_state = false;
            try{
                default_off_on_state = this.BackendImplementations[(int)this.TechnologyIndex].GetOffOnState();
            } catch(Exception ex){
                this.openMessageWindow(ex.Message);
                return;
            }
            if(default_off_on_state){
                turn_off_on_button.Content = "Turn motor off";
                get_on_off_state.Text = "Motor is turned on!";
            } else {
                turn_off_on_button.Content = "Turn motor on";
                get_on_off_state.Text = "Motor is turned off!";
            }
        }

        private void enableDisableGuiElementsSetSpeedPos(){
            TextBox? get_mode_textbox = this.Find<TextBox>("GetModeText");
            StackPanel? set_speed_functionality = this.Find<StackPanel>("SetSpeedPanel");
            StackPanel? set_pos_functionality = this.Find<StackPanel>("SetPosPanel");
            if(get_mode_textbox is null || set_speed_functionality is null || set_pos_functionality is null){
                this.openMessageWindow($"{get_mode_textbox} or {set_speed_functionality} or {set_pos_functionality} doesn't exist!");
                return;
            }

            if(get_mode_textbox.Text == "Speed"){
                set_speed_functionality.IsEnabled = true;
                set_pos_functionality.IsEnabled = false;
            } else if(get_mode_textbox.Text == "Position"){
                set_speed_functionality.IsEnabled = false;
                set_pos_functionality.IsEnabled = true;
            }
        }

        private void disableStackPanels(List<string> namesOfPanels){
            foreach (string stackPanel in namesOfPanels){
                StackPanel? stackPanelObject = this.Find<StackPanel>(stackPanel);
                if(stackPanelObject is null){
                    this.openMessageWindow($"{stackPanelObject} doesn't exist!");
                    return;
                }

                stackPanelObject.IsEnabled = false;
            }
        }

        private void openMessageWindow(string message){
            try{
                MessageBox.Show(this, message, "ERROR", MessageBox.MessageBoxButtons.Ok);
            } catch(System.InvalidOperationException){

            }
        }

        // TODO - figure out why these two doesn't work
        // private T Find<T>(string panel_elem_name) where T : class{
        //     T? set_mode_combo = this.Find<T>(panel_elem_name);
        //     if(set_mode_combo is null){
        //         this.openMessageWindow($"{panel_elem_name} doesn't exist!");
        //         Logger?.Error($"{panel_elem_name} doesn't exist!");
        //         throw new ArgumentNullException();
        //     }
        //     return (T)set_mode_combo;
        // }

        // private IHostappBackend dereferenceTechnology(){
        //     if(this.TechnologyIndex is null){
        //         this.openMessageWindow("Technology is null!");
        //         Logger?.Error("Technology is null!");
        //         throw new ArgumentNullException();
        //     }

        //     return this.BackendImplementations[(int)this.TechnologyIndex];
        // }
    }
}
