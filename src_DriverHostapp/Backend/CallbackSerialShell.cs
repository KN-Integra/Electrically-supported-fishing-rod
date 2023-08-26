using System;
using System.IO.Ports;//dotnet add package System.IO.Ports --version 7.0.0

using System.Collections;
using System.Collections.Generic;

using DriverHostapp.Backend.CallbackInterface;
using DriverHostapp.Backend.Utils.SerialConnectionDefinition;
using DriverHostapp.Backend.Utils.ErrorCodes;
using DriverHostapp.Backend.Utils.ControlModes;

namespace DriverHostapp.Backend.CallbackSerialShell{
    public class HostappBackendSerial : IHostappBackend{

        private List<SerialDriverConnection> ListOfDevices;
        private uint? connection_index;

        public HostappBackendSerial(){
            this.ListOfDevices = new List<SerialDriverConnection>();
            connection_index = null;
        }

        public void list_devices(){
            string[] ports = SerialPort.GetPortNames();

            this.ListOfDevices = new List<SerialDriverConnection>();

            // Display each port name to the console.
            foreach(string port in ports){ 
                try{
                    this.ListOfDevices.Add(this.get_serial_device_info(port));
                } catch (WrongDevice){
                } catch (System.UnauthorizedAccessException){
                }
                
            }
            
        }

        public List<string> get_connections_as_string_list(){
            List<string> devs_as_strings = new List<string>();
            uint i = 0;
            foreach (SerialDriverConnection item in this.ListOfDevices){
                string p = item.Port;
                string hw_info = item.HardwareID;
                string sw_info = item.SoftwareVersion;
                devs_as_strings.Add($"{i}: {p}, sw version: {sw_info} (device id: {hw_info})");
                i+=1;
            }
            return devs_as_strings;
        }

        private SerialDriverConnection get_serial_device_info(string port_name){

            SerialDriverConnection serial_connection = new SerialDriverConnection(port_name, "", "");

            serial_connection.open_serial_port();


            serial_connection.write_data("hwinfo devid\n");
            List<string> lines = serial_connection.read_lines();

            string final_id = "";
            foreach (var l in lines){ 
                if(l.Contains("ID: ")){
                    final_id = l.Substring(4);
                    break;
                }                
            }

            if(final_id == ""){
                throw new WrongDevice($"{port_name} doesn't implement hwinfo devid command!");
            }

            serial_connection.HardwareID = final_id;


            

            serial_connection.write_data("drv_version\n");
            lines = serial_connection.read_lines();
            string final_sw_ver = "";

            foreach (var l in lines){
                if(l.Contains("Software version: ")){
                    final_sw_ver = l.Substring(25);// 25 = "Software version: ".Count()
                    break;
                }
            }

            if(final_sw_ver == ""){
                throw new WrongDevice($"{port_name} doesn't implement drv_version command!");
            }

            serial_connection.SoftwareVersion = final_sw_ver;


            serial_connection.close_serial_port();

            return serial_connection;
        }

        public void choose_connection_by_index(uint index){
            if(index >= this.ListOfDevices.Count){
                throw new NonExistingDevice($"id. {index} is not in the list of possible Serial connections!");
            }
            this.connection_index = index;            
        }

        public void open_connection(){
            if(this.connection_index is null){
                throw new DeviceNotChosen("Cannot open connection if no connection was chosen!");
            }
            this.ListOfDevices[(int)this.connection_index].open_serial_port();
        }

        public void send_configuration(){
            if(this.connection_index is null){
                throw new DeviceNotChosen("Cannot send configuraton if no connection was chosen!");
            }

            this.ListOfDevices[(int)this.connection_index].write_data("init\n");
            this.get_return_from_device_with_error_check("initialisation succesful!");
        }

        public void set_mode(ControlMode new_mode){
            if(connection_index is null){
                throw new DeviceNotChosen("Cannot send mode if no connection was chosen!");
            }
            this.ListOfDevices[(int)this.connection_index].write_data($"mode {new_mode.ToString().ToLower()}\n");
            this.get_return_from_device_with_error_check($"mode set to: {new_mode.ToString().ToLower()}");
        }
        public ControlMode get_mode(){
            if(connection_index is null){
                throw new DeviceNotChosen("Cannot send speed if no connection was chosen!");
            }
            this.ListOfDevices[(int)this.connection_index].write_data($"mode\n");
            List<string> lines = this.ListOfDevices[(int)this.connection_index].read_lines();

            string error_code = "";

            foreach (string l in lines){
                if(l.Contains("mode: ")){
                    if(l.Substring(6) == "0"){
                        return ControlMode.Speed;
                    } else if(l.Substring(6) == "1"){
                        return ControlMode.Position;
                    }
                } else{
                    error_code = l;
                }
            }

            return ControlMode.Speed;
        }

        public void set_speed(uint target_speed_in_mrpm){
            if(connection_index is null){
                throw new DeviceNotChosen("Cannot send speed if no connection was chosen!");
            }
            this.ListOfDevices[(int)this.connection_index].write_data($"speed {target_speed_in_mrpm}\n");
            this.get_return_from_device_with_error_check($"speed set to: {target_speed_in_mrpm}");
        }

        public uint get_speed(){
            if(connection_index is null){
                throw new DeviceNotChosen("Cannot get speed if no connection was chosen!");
            }

            this.ListOfDevices[(int)this.connection_index].write_data($"speed\n");

            List<string> lines = this.ListOfDevices[(int)this.connection_index].read_lines();

            string error_code = "";

            foreach (string l in lines){
                if(l.Contains("speed: ")){
                    return Convert.ToUInt32(l.Substring(7));
                } else{
                    error_code = l;
                }
            }

            throw new ErrorFromDriver(error_code);
        }

        public void set_position(){
            // NOT YET IMPLEMENTED
        }

        public uint get_position(){
            // NOT YET IMPLEMENTED
            return 0u;
        }

        public void turn_driver_on(){
            if(connection_index is null){
                throw new DeviceNotChosen("Cannot get speed if no connection was chosen!");
            }

            this.ListOfDevices[(int)this.connection_index].write_data("start --on\n");
            this.get_return_from_device_with_error_check("Operation executed!");
        }

        public void turn_driver_off(){
            if(connection_index is null){
                throw new DeviceNotChosen("Cannot get speed if no connection was chosen!");
            }

            this.ListOfDevices[(int)this.connection_index].write_data("start --off\n");
            this.get_return_from_device_with_error_check("Operation executed!");
        }
        
        public bool get_off_on(){
            if(connection_index is null){
                throw new DeviceNotChosen("Cannot get speed if no connection was chosen!");
            }

            this.ListOfDevices[(int)this.connection_index].write_data("start --get\n");

            List<string> lines = this.ListOfDevices[(int)this.connection_index].read_lines();
            string error_code = "";
            foreach (string l in lines){
                if(l.Contains("Motor is turned on!")){
                    return true;
                } else if(l.Contains("Motor is turned off!")){
                    return false;
                } else{ 
                    error_code = l;
                }
            }
            throw new ErrorFromDriver(error_code);
        }

        public void close_connection(){
            if(connection_index is null){
                throw new DeviceNotChosen("Cannot close connection if no connection was chosen!");
            }
            this.ListOfDevices[(int)this.connection_index].close_serial_port();
        }

        private void get_return_from_device_with_error_check(string expected_string){
            if(connection_index is null){
                throw new DeviceNotChosen("Cannot send speed if no connection was chosen!");
            }

            var return_lines = this.ListOfDevices[(int)this.connection_index].read_lines();

            string error_code = "";

            foreach (var l in return_lines){
                if(l == expected_string){
                    return;
                } else{
                    error_code = l;
                }
            }

            throw new ErrorFromDriver(error_code);
        }
    }
}
