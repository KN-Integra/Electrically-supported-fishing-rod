namespace driver_hostapp.backend.callback_interface{
    public interface IHostappBackend{
        void list_devices();

        List<string> get_connections_as_string_list();
        void choose_connection_by_index(uint index);

        void open_connection();

        void send_configuration();

        void set_mode();
        void get_mode();

        void set_speed(uint target_speed_in_mrpm);
        uint get_speed();

        void set_position();
        void get_position();

        void close_connection();
    }
}
