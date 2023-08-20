using System;

namespace DriverHostapp.Backend.Utils.ErrorCodes{
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

    [Serializable]
    public class ErrorFromDriver : DriverException{
        public ErrorFromDriver(string message)
            : base(message[7..]){} // remove uart prefix

        // private string removeUartPrefix(string message){
        //     return message[7..];
        // }
    }

    [Serializable]
    public class ReinitConnection : DriverException{
        public ReinitConnection(string message)
            : base(message){}
    }
}
