using System.IO.Ports; //dotnet add package System.IO.Ports --version 7.0.0

using driver_hostapp.backend.callback_interface;

namespace driver_hostapp.backend.callback_serial{

    // public class NotNordicDevice : Exception
    // {

    //     public EmployeeListNotFoundException(string message)
    //         : base(message)
    //     {
    //     }
    // }

    public class HostappBackendSerial : IHostappBackend{

        public void list_devices(){
            string[] ports = SerialPort.GetPortNames();

            Console.WriteLine("The following serial ports were found:");

            // Display each port name to the console.
            foreach(string port in ports)
            {
                Console.WriteLine(port);
                this.get_serial_device_info(port);
            }
            
        }

        private void get_serial_device_info(string port_name){
            var str_to_write = "hwinfo devid\n";            

            SerialPort mySerialPort = new SerialPort(port_name);
            mySerialPort.BaudRate = 9600;
            mySerialPort.RtsEnable = true; // thefuck?
            mySerialPort.DtrEnable = true; // thefuck?

            mySerialPort.ReadTimeout = 100;

            mySerialPort.Open();

            mySerialPort.Write(str_to_write);

            string final_id = "";

            for (int i = 0; i < 3; i++)
            {
                try{
                    string l = mySerialPort.ReadLine();
                    if(l.Contains("ID: ")){
                        Console.WriteLine(l);
                    }
                }
                catch (System.TimeoutException)
                {

                }
                
            }

            // if(final_id == ""){
            //     throw
            // }

            mySerialPort.Close();
        }

        public void init_connection(){

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
    }
}