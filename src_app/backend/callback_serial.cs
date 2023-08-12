 using System.IO.Ports;//dotnet add package System.IO.Ports --version 7.0.0

using driver_hostapp.backend.callback_interface;
using driver_hostapp.backend.utils.serial_connection_definition;
using driver_hostapp.backend.utils.error_codes;

namespace driver_hostapp.backend.callback_serial{
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
            if(index >= this.ListOfDevices.Count()){
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
            this.ListOfDevices[(int)this.connection_index].read_lines();
        }

        public void set_mode(){

        }
        public void get_mode(){

        }

        public void set_speed(uint target_speed_in_mrpm){
            if(connection_index is null){
                throw new DeviceNotChosen("Cannot send speed if no connection was chosen!");
            }
            this.ListOfDevices[(int)this.connection_index].write_data($"speed {target_speed_in_mrpm}\n");
            this.ListOfDevices[(int)this.connection_index].read_lines();
        }
        public void get_speed(){

        }

        public void set_position(){

        }
        public void get_position(){

        }

        public void close_connection(){
            if(connection_index is null){
                throw new System.Exception(); // TODO - change exception!!!
            }
            this.ListOfDevices[(int)this.connection_index].close_serial_port();
        }
    }
}
