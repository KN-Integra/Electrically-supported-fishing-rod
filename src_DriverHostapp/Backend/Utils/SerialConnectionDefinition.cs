using System.IO.Ports;

using System.Collections;
using System.Collections.Generic;

using DriverHostapp.Backend.Utils.ErrorCodes;

namespace DriverHostapp.Backend.Utils.SerialConnectionDefinition{
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
            if(this.SerialConnection is not null){
                throw new ReinitConnection("connection already opened!");
            }

            this.SerialConnection = new SerialPort(this.Port);
            this.SerialConnection.BaudRate = 9600;
            this.SerialConnection.RtsEnable = true;
            this.SerialConnection.DtrEnable = true;
            // TODO - what does this two options do?

            this.SerialConnection.ReadTimeout = 100;
            this.SerialConnection.WriteTimeout = 100;

            this.SerialConnection.Open();
        }

        public void write_data(string str_of_data){
            if(this.SerialConnection is not null){
                try{
                    this.SerialConnection.Write(str_of_data);
                } catch {
                    throw new WrongDevice($"Could not write to this device");
                }
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
                        recieved_lines.Add(l.Substring(0, l.Length-1));
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
                if(!this.SerialConnection.IsOpen){
                    throw new DeviceNotConnected($"Open Serial device {this.Port} before closing attempt!");
                }
                this.SerialConnection.Close();
                this.SerialConnection = null;
             }else {
                throw new DeviceNotConnected($"Open Serial device {this.Port} before closing attempt!");
            }
        }

    }
}
