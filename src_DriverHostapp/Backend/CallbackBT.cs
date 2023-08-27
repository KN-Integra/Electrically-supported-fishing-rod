using System.Collections;
using System.Collections.Generic;

using DriverHostapp.Backend.CallbackInterface;
using DriverHostapp.Backend.Utils.ControlModes;

// STUB
// NOT YET IMPLEMENTED

namespace DriverHostapp.Backend.CallbackBT{
    public class HostappBackendBluetooth : IHostappBackend{

        public NLog.Logger? Logger = null;

        public void list_devices(){
        }

        public List<string> get_connections_as_string_list(){
            return new List<string>();
        }

        public void choose_connection_by_index(uint index){

        }

        public void open_connection(){

        }

        public void send_configuration(){

        }

        public void set_mode(ControlMode new_mode){

        }
        public ControlMode get_mode(){
            return ControlMode.Speed;
        }

        public void set_speed(uint target_speed_in_mrpm){

        }
        public uint get_speed(){
            return 0u;
        }

        public void set_position(){

        }
        public uint get_position(){
            return 0u;
        }

        public void turn_driver_on(){

        }

        public void turn_driver_off(){

        }

        public bool get_off_on(){
            return false;
        }

        public void close_connection(){

        }

        public void SetLogger(NLog.Logger? Logger){
            this.Logger = Logger;
        }
    }
}
