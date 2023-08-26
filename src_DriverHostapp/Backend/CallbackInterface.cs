using System.Collections;
using System.Collections.Generic;
using DriverHostapp.Backend.Utils.ControlModes;

namespace DriverHostapp.Backend.CallbackInterface{
    public interface IHostappBackend{
        void list_devices();

        List<string> get_connections_as_string_list();
        void choose_connection_by_index(uint index);

        void open_connection();

        void send_configuration();

        void set_mode(ControlMode new_mode);
        ControlMode get_mode();

        void set_speed(uint target_speed_in_mrpm);
        uint get_speed();

        void set_position();
        uint get_position();

        void turn_driver_on();
        void turn_driver_off();

        bool get_off_on();

        void close_connection();
    }
}
