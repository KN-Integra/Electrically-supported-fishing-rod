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
            implementations[i].init_connection();

        }
    }
}
