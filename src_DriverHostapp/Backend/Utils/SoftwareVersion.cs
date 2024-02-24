namespace DriverHostapp.Backend.Utils.SoftwareVersion;

public class SoftwareVersion{
    public uint MajorVersion { get; }
    public uint MinorVersion { get; }

    public SoftwareVersion(uint majorVersion, uint minorVersion){
        MajorVersion= majorVersion;
        MinorVersion = minorVersion;
    }
}
