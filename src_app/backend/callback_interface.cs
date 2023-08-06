namespace driver_hostapp.backend.callback_interface{
    public interface IHostappBackend{
        void list_devices();

        void init_connection();

        void send_configuration();

        void set_mode();
        void get_mode();

        void set_speed();
        void get_speed();

        void set_position();
        void get_position();
    }
}