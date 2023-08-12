using System.IO.Ports;

using driver_hostapp.backend.utils.error_codes;

namespace driver_hostapp.backend.utils.serial_connection_definition{
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
            } else {
                throw new DeviceNotConnected($"Open Serial device {this.Port} before write attempt!");
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
                throw new DeviceNotConnected($"Open Serial device {this.Port} before read attempt!");
            }
        }

        public void close_serial_port(){
            if(this.SerialConnection is not null){
                this.SerialConnection.Close();
                this.SerialConnection = null;
             }else {
                throw new DeviceNotConnected($"Open Serial device {this.Port} before closing attempt!");
            }
        }

    }
}