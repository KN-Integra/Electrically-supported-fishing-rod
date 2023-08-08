using System.IO.Ports; //dotnet add package System.IO.Ports --version 7.0.0

using driver_hostapp.backend.callback_interface;

namespace driver_hostapp.backend.callback_serial{
    [Serializable]
    public class NotNordicDevice : Exception{
        public NotNordicDevice(string message)
            : base(message){}
    }

    public class SerialDriverConnection{
        public string Port;
        public string HardwareID;
        public string SoftwareVersion;

        private SerialPort? SerialConnection;
        public SerialDriverConnection(string port, string hw_id, string sw_ver){
            this.Port = port;
            this.HardwareID = hw_id;
            this.SoftwareVersion = sw_ver;
            this.SerialConnection = null;
        }

        public void open_serial_port(){
            this.SerialConnection = new SerialPort(this.Port);
            this.SerialConnection.BaudRate = 9600;
            this.SerialConnection.RtsEnable = true; // thefuck?
            this.SerialConnection.DtrEnable = true; // thefuck?

            this.SerialConnection.ReadTimeout = 100;

            this.SerialConnection.Open();
        }

        public void write_data(string str_of_data){
            if(this.SerialConnection is not null){
                this.SerialConnection.Write(str_of_data);
            }
        }

        public List<string> read_lines(){
            if(this.SerialConnection is not null){
                List<string> recieved_lines = new List<string>();
                while (true){
                    try{
                        string l = this.SerialConnection.ReadLine();
                        recieved_lines.Add(l.Substring(0, l.Count()-1));
                    } catch (System.TimeoutException){
                        break;
                    }
                }

                return recieved_lines;
            } else {
                throw new System.Exception(); // TODO - error!
            }
        }

        public void close_serial_port(){
            if(this.SerialConnection is not null){
                this.SerialConnection.Close();
            }
        }

    }

    public class HostappBackendSerial : IHostappBackend{

        private List<SerialDriverConnection> ListOfDevices;
        private uint? connection_index;

        public HostappBackendSerial(){
            ListOfDevices = new List<SerialDriverConnection>();
            connection_index = null;
        }

        public void list_devices(){
            string[] ports = SerialPort.GetPortNames();

            ListOfDevices = new List<SerialDriverConnection>();

            // Display each port name to the console.
            foreach(string port in ports){ 
                try{
                    ListOfDevices.Add(this.get_serial_device_info(port));
                } catch (NotNordicDevice){
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
                throw new NotNordicDevice($"{port_name} doesn't implement hwinfo devid command!");
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
                throw new NotNordicDevice($"{port_name} doesn't implement drv_version command!");
            }

            serial_connection.SoftwareVersion = final_sw_ver;


            serial_connection.close_serial_port();

            return serial_connection;
        }

        public void choose_connection_by_index(uint index){
            this.connection_index = index;
        }

        public void open_connection(){
            if(connection_index is null){
                throw new System.Exception(); // TODO - change exception!!!
            }
            this.ListOfDevices[(int)this.connection_index].open_serial_port();
        }

        public void send_configuration(){

        }

        public void set_mode(){

        }
        public void get_mode(){

        }

        public void set_speed(){

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
