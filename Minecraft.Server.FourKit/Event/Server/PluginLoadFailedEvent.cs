namespace Minecraft.Server.FourKit.Event.Server;

using Minecraft.Server.FourKit.Plugin;

public class PluginLoadFailedEvent : ServerEvent
{
    private readonly string _fileName;
    private readonly string _message;
    internal PluginLoadFailedEvent(string fileName, string message) : base()
    {
        _fileName = fileName;
        _message = message;
    }

    public string getFileName() => _fileName;

    public string getMessage() => _message;
}
