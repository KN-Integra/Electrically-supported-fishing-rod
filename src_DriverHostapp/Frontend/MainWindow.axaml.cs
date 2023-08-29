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

namespace DriverHostapp.Frontend.HostappMainWindow{
    public partial class MainWindow : Window{
        private uint? TechnologyIndex;
        private List<IHostappBackend> BackendImplementations;

        public NLog.Logger? Logger = null;

        public MainWindow(){
            BackendImplementations = new List<IHostappBackend>();

            BackendImplementations.Add(new HostappBackendSerial());
            BackendImplementations.Add(new HostappBackendBluetooth());
            
            TechnologyIndex = null;

            // this.EnableLogging();

            // Generated with Avalonia.NameGenerator            
            InitializeComponent();
        }

         public void OpenAppSettings(object sender, RoutedEventArgs args){
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
            if(this.TechnologyIndex is not null){
                this.BackendImplementations[(int)this.TechnologyIndex].SetLogger(this.Logger);
            }

            Logger.Info("Logger Starting!");
        }

        public void DisableLogging(){
            Logger?.Info("Logger Stoping!");
            Logger = null;
        }

        public void ConnType_SelectionChanged(object sender, SelectionChangedEventArgs e){
            ComboBox comboBox = (ComboBox)sender;

            if(this.TechnologyIndex is not null){
                try{
                    this.BackendImplementations[(int)this.TechnologyIndex].CloseConnection();
                } catch(DeviceNotConnected){
                    //TODO logging
                } catch(Exception){
                    // TODO - logging
                }
            }

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
            if(button is not null && panel is not null){
                panel.IsEnabled = (comboBox.SelectedIndex != 0);
                ComboBoxItem? singleItem = (ComboBoxItem?)comboBox.SelectedItem;
                if(singleItem is not null){
                    if(panel.IsEnabled){
                        button.Content = $"Scan {singleItem.Content} devices";
                    } else{
                        button.Content = $"Scan devices";
                        ComboBox? comboBox_devSelection = this.Find<ComboBox>("SelectDeviceComboBox");
                        StackPanel? panel_devSelection = this.Find<StackPanel>("SelectDevicePanel");
                        if(comboBox_devSelection is not null && panel_devSelection is not null){
                            panel_devSelection.IsEnabled = false;
                            comboBox_devSelection.Items.Clear();
                            comboBox_devSelection.SelectedIndex = 0;
                        }
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
            }


        }

        public void ScanDevices(object sender, RoutedEventArgs args){
            ComboBox? comboBox = this.Find<ComboBox>("SelectDeviceComboBox");
            StackPanel? panel = this.Find<StackPanel>("SelectDevicePanel");
            if(comboBox is not null && panel is not null){
                comboBox.Items.Clear();
                panel.IsEnabled = true;
                comboBox.Items.Add("Select Device");
                if(this.TechnologyIndex is not null){
                    try{
                        this.BackendImplementations[(int)this.TechnologyIndex].ListDevices();
                        List<string> connected_device_list = this.BackendImplementations[(int)this.TechnologyIndex].GetConnectionsAsListOfStrings();
                        foreach(string s in connected_device_list){
                            comboBox.Items.Add(s);
                        }
                    } catch(Exception ex){
                        Logger?.Info($"Selected Technology {ex.Message}");
                    }
                }
                comboBox.SelectedIndex = 0;
            }
        }

        

        public void SelectDevice_SelectionChanged(object sender, SelectionChangedEventArgs e){
            ComboBox comboBox = (ComboBox)sender;
            Console.WriteLine("0");

            StackPanel? connect_to_dev_panel = this.Find<StackPanel>("ConnectToDevicePanel");
            if(connect_to_dev_panel is not null){
                connect_to_dev_panel.IsEnabled = (comboBox.SelectedIndex != 0);
            }
            if(this.TechnologyIndex is not null){
                if(comboBox.SelectedIndex > 0){
                    try{
                        this.BackendImplementations[(int)this.TechnologyIndex].ChooseConnectionByIndex((uint)comboBox.SelectedIndex-1);
                    } catch(Exception ex){
                        this.openMessageWindow(ex.Message);
                        return;
                    }                    
                }
            }

            if(comboBox.SelectedIndex == 0 && this.TechnologyIndex is not null){
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

        public void ConnectDevice(object sender, RoutedEventArgs args){
            StackPanel? button = this.Find<StackPanel>("SendInitPanel");
            if(button is not null){
                button.IsEnabled = true;
            }

            if(this.TechnologyIndex is not null){
                try{
                    this.BackendImplementations[(int)this.TechnologyIndex].OpenConnection();
                } catch(Exception ex){
                    this.openMessageWindow(ex.Message);
                    return;
                }
            }
        }

        public void SetConnectionOptions(object sender, RoutedEventArgs args){
            var window = new ConnectionOptions();
            window.Show();
        }


        public void Init(object sender, RoutedEventArgs args){
            if(this.TechnologyIndex is not null){
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
                if(get_speed_textbox is not null){
                    get_speed_textbox.Text = default_speed.ToString();
                }

                TextBox? get_pos_textbox = this.Find<TextBox>("GetPosText");
                if(get_pos_textbox is not null){
                    get_pos_textbox.Text = default_position.ToString();
                }
            } else{
                return;
            }

            List<string> panels_to_enable = new List<string>{
                "SetModePanel",
                "GetModePanel",
                "GetSpeedPanel",
                "GetPosPanel",
                "OffOnPanel"
            };

            foreach (string panel_name in panels_to_enable){
                StackPanel? enabled_functionality = this.Find<StackPanel>(panel_name);
                if(enabled_functionality is not null){
                    enabled_functionality.IsEnabled = true;
                }
            }


            this.enableDisableGuiElementsOffOn();
            this.enableDisableGuiElementsSetSpeedPos();
        }

        public void SetInitParams(object sender, RoutedEventArgs args){
            var window = new InitOptions();
            window.Show();
        }

        public void GetMode(object sender, RoutedEventArgs args){
            updateModeTextBox();
        }

        private void updateModeTextBox(){
            TextBox? get_mode_textbox = this.Find<TextBox>("GetModeText");
            if(get_mode_textbox is not null){
                if(this.TechnologyIndex is not null){
                    ControlMode recievedControlMode = ControlMode.Speed;
                    try{
                        recievedControlMode = this.BackendImplementations[(int)this.TechnologyIndex].GetMode();
                    } catch(Exception ex){
                        this.openMessageWindow(ex.Message);
                        return;
                    }
                    get_mode_textbox.Text = recievedControlMode.ToString();
                }
            }
        }

        public void SetMode(object sender, RoutedEventArgs args){
            ComboBox? set_mode_combo = this.Find<ComboBox>("SendModeComboBox");
            if(set_mode_combo is not null){
                ComboBoxItem? single_item = (ComboBoxItem?)set_mode_combo.Items[set_mode_combo.SelectedIndex];
                ControlMode value_to_set;
                if(single_item is not null){
                    if(single_item.Content is not null){
                        if (!Enum.TryParse(single_item.Content.ToString(), out value_to_set)){
                            this.openMessageWindow("Value from mode ComboBox could not be parsed into Enum!");
                            return;
                        }
                    } else{
                        this.openMessageWindow("Selected ComboBox item doesn't have conent!");
                        return;
                    }
                } else{
                    this.openMessageWindow("No ComboBox item chosen or chosen item is null!");
                    return;
                }

                if(this.TechnologyIndex is not null){
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
                    } else{
                        this.updateModeTextBox();
                        this.enableDisableGuiElementsSetSpeedPos();
                    }
                }
            }
        }
        
        public void GetSpeed(object sender, RoutedEventArgs args){
            if(this.TechnologyIndex is not null){
                uint default_speed = 0;

                try{
                    default_speed = this.BackendImplementations[(int)this.TechnologyIndex].GetSpeed();
                } catch(Exception ex){
                    this.openMessageWindow(ex.Message);
                    return;
                }

                TextBox? get_speed_textbox = this.Find<TextBox>("GetSpeedText");
                if(get_speed_textbox is not null){
                    get_speed_textbox.Text = default_speed.ToString();
                }
            }
        }

        public void SetSpeed(object sender, RoutedEventArgs args){
            uint speed_to_set = 0u;
            TextBox? set_speed_textbox = this.Find<TextBox>("SetSpeedText");
            if(set_speed_textbox is not null){
                if(!UInt32.TryParse(set_speed_textbox.Text, out speed_to_set)){
                    this.openMessageWindow("Parsing error, non numeric value in text box!");
                    return;
                }
            }

            if(this.TechnologyIndex is not null){
                try{
                    this.BackendImplementations[(int)this.TechnologyIndex].SetSpeed(speed_to_set);
                } catch(Exception ex){
                    this.openMessageWindow(ex.Message);
                    return;
                }                
            }
        }

        public void GetPosition(object sender, RoutedEventArgs args){
            if(this.TechnologyIndex is not null){
                uint default_position = this.BackendImplementations[(int)this.TechnologyIndex].GetPosition();

                TextBox? get_pos_textbox = this.Find<TextBox>("GetPosText");
                if(get_pos_textbox is not null){
                    get_pos_textbox.Text = default_position.ToString();
                }
            }
        }

        public void SetPosition(object sender, RoutedEventArgs args){

        }

        public void OffOnDevice(object sender, RoutedEventArgs args){
            if(this.TechnologyIndex is not null){
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
            }


            this.enableDisableGuiElementsOffOn();
        }

        public void GetOffOnState(object sender, RoutedEventArgs args){
            this.enableDisableGuiElementsOffOn();
        }

        private void enableDisableGuiElementsOffOn(){
            Button? turn_off_on_button = this.Find<Button>("OffOnButton");
            TextBox? get_on_off_state = this.Find<TextBox>("OffOnText");
            if(this.TechnologyIndex is not null && get_on_off_state is not null && turn_off_on_button is not null){
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
        }

        private void enableDisableGuiElementsSetSpeedPos(){
            TextBox? get_mode_textbox = this.Find<TextBox>("GetModeText");
            if(get_mode_textbox is not null){
                StackPanel? set_speed_functionality = this.Find<StackPanel>("SetSpeedPanel");
                StackPanel? set_pos_functionality = this.Find<StackPanel>("SetPosPanel");
                if(get_mode_textbox.Text == "Speed"){
                    if(set_speed_functionality is not null){
                        set_speed_functionality.IsEnabled = true;
                    }
                    if(set_pos_functionality is not null){
                        set_pos_functionality.IsEnabled = false;
                    }
                } else if(get_mode_textbox.Text == "Position"){
                    if(set_speed_functionality is not null){
                        set_speed_functionality.IsEnabled = false;
                    }
                    if(set_pos_functionality is not null){
                        set_pos_functionality.IsEnabled = true;
                    }
                }
            }
        }

        private void disableStackPanels(List<string> namesOfPanels){
            foreach (string stackPanel in namesOfPanels){
                StackPanel? stackPanelObject = this.Find<StackPanel>(stackPanel);
                if(stackPanelObject is not null){
                    stackPanelObject.IsEnabled = false;
                }
            }
        }

        private void openMessageWindow(string message){
            MessageBox.Show(this, message, "ERROR", MessageBox.MessageBoxButtons.Ok);
        }
    }
}
