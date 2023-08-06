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

            int i = 1;

            implementations[i].list_devices();

            // UInt32 set = 10000;
            // byte[] s = new byte[6];
            
            // s[0] = 0x21;

            // for(int i =4; i> 0; --i){
            //     s[i] = Convert.ToByte(set & 0xff);
            //     set = set >> 8;
            // }

            // s[5] = Convert.ToByte('\n');
            

            // SerialPort mySerialPort = new SerialPort("COM9");
            // mySerialPort.BaudRate = 9600;

            // mySerialPort.Open();

            // mySerialPort.Write(s, 0, s.Length);

            // mySerialPort.Close();

            // foreach(var item in s)
            // {
            //     Console.WriteLine(item.ToString());
            // }
        }
    }
}
