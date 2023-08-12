namespace driver_hostapp.backend.utils.error_codes{
    [Serializable]
    public class DriverException : Exception{
        public DriverException(string message)
            : base(message){}
    }


    [Serializable]
    public class WrongDevice : DriverException{
        public WrongDevice(string message)
            : base(message){}
    }

    [Serializable]
    public class NonExistingDevice : DriverException{
        public NonExistingDevice(string message)
            : base(message){}
    }

    [Serializable]
    public class DeviceNotChosen : DriverException{
        public DeviceNotChosen(string message)
            : base(message){}
    }


    [Serializable]
    public class DeviceNotConnected : DriverException{
        public DeviceNotConnected(string message)
            : base(message){}
    }
}