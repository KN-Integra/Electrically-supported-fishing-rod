using driver_hostapp.backend.callback_interface;

namespace driver_hostapp.backend.callback_bt{
    public class HostappBackendBluetooth : IHostappBackend{

        public void list_devices(){
            Console.WriteLine("bt init");
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