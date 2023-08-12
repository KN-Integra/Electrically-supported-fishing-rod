using System;
using System.IO.Ports;
using System.ComponentModel;

using driver_hostapp.backend.callback_interface;
using driver_hostapp.backend.callback_serial;
using driver_hostapp.backend.callback_bt;

namespace Chasztag_sender_app
{
    class Program
    {
        static void Main(string[] args){


            List<IHostappBackend> implementations = new List<IHostappBackend>();

            implementations.Add(new HostappBackendSerial());
            implementations.Add(new HostappBackendBluetooth());

            int i = 0;

            implementations[i].list_devices();

            Console.WriteLine("The following serial ports were found:");
            foreach (string item in implementations[i].get_connections_as_string_list()){
                Console.WriteLine(item);
            }

            implementations[i].list_devices();

            implementations[i].choose_connection_by_index(0u);

            implementations[i].open_connection();

            implementations[i].send_configuration();
            implementations[i].set_speed(0000u);


            implementations[i].close_connection();

        }
    }
}
