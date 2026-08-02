namespace Minecraft.Server.FourKit;

internal static class ServerLog
{
    private static string Timestamp() =>
        DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss.fff");

    public static void Debug(string category, string message) =>
        Console.WriteLine($"[{Timestamp()}][DEBUG][{category}] {message}");

    public static void Info(string category, string message) =>
        Console.WriteLine($"[{Timestamp()}][INFO][{category}] {message}");

    public static void Warn(string category, string message) =>
        Console.WriteLine($"[{Timestamp()}][WARN][{category}] {message}");

    public static void Error(string category, string message) =>
        Console.WriteLine($"[{Timestamp()}][ERROR][{category}] {message}");
}
