@page plugin-creation Creating your first Plugin

@section main-plugin Initialization

This will go over how to create your first plugin.

If you havent already, be sure to set up your development environment first:

@ref setup

Plugins must have a class that extends \ref Minecraft.Server.FourKit.Plugin.ServerPlugin "ServerPlugin".

```csharp
using Minecraft.Server.FourKit;
using Minecraft.Server.FourKit.Plugin;

public class CoolPlugin : ServerPlugin
{
    public override string name => "My Cool Plugin";
    public override string version => "1.0.0";
    public override string author => "Me";

    public override void onEnable() { }

    public override void onDisable() { }
}
```

`onEnable()` is ran when the server starts. This is where you add listeners and anything else you need to do on startup.

`onDisable()` runs when the server stops. You can do stuff like cleaning up here.

@section listeners Listeners

Listeners are vital for events to be intercepted by your plugin. This will go over the usage and how to get started.

Listeners must implement the \ref Minecraft.Server.FourKit.Event.Listener "Listener" interface. Your listener class should look like this:

```csharp
using Minecraft.Server.FourKit.Event;

public class MyListener : Listener
{

}
```

### Registering your listener

To register a listener, you need to add it to FourKit, a common place to do this is in `onEnable()` in your plugin.

```csharp
public override void onEnable() { 
    FourKit.addListener(new MyListener());
}
```

### Listening to Events

Now that we've registered the listener, we need to make it actually listen to events!

To listen to any given event in your listener class, you MUST create a method with the \ref Minecraft.Server.FourKit.Event.EventHandlerAttribute "EventHandler" attribute attached and the event specified by the type in the methods argument. The method can be named whatever you wish. Example:

```csharp
using Minecraft.Server.FourKit.Event;
using Minecraft.Server.FourKit.Event.Player;

public class MyListener : Listener
{
    [EventHandler]
    public void onPlayerJoin(PlayerJoinEvent e) {

    }
}
```

This method will fire whenever a player joins the server. We can make it broadcast a greeting to the whole server:

```csharp
using Minecraft.Server.FourKit.Event;
using Minecraft.Server.FourKit.Event.Player;

public class MyListener : Listener
{
    [EventHandler]
    public void onPlayerJoin(PlayerJoinEvent e) {
        FourKit.broadcastMessage("Welcome!");
    }
}
```

### Manipulating Events

You may modify what happens with most events and also obtain information about the given event. These functions are stored in the Event object in your method. Let's modify the message that is broadcasted when a player joins the server:

```csharp
using Minecraft.Server.FourKit.Event;
using Minecraft.Server.FourKit.Event.Player;

public class MyListener : Listener
{
    [EventHandler]
    public void onPlayerJoin(PlayerJoinEvent e) {
        event.setJoinMessage("Welcome, " + event.getPlayer().getName() + "!");
    }
}
```

### What can I listen to?

You can browse through the \ref Minecraft.Server.FourKit.Event "Event" namespace to see all events that you can use.

@ref usage-of-all-events

@section advanced-functions Advanced Functions

### EventHandler parameters

The EventHandler attribute accepts a couple parameters.

**Priority** - indicates the priority. There are six different priorities, in order of execution:

- `Lowest`
- `Low`
- `Normal` (Default)
- `High`
- `Highest`
- `Monitor`

These constants refer to the \ref Minecraft.Server.FourKit.Event.EventPriority "EventPriority" enum.

<b><span style="color: red;">Note:</span> The Monitor priority should only be used for reading only. This priority is useful for logging plugins to see the results of an event and modifying values may interfere with those types of plugins.</b>

**IgnoreCancelled** - A boolean which indicates whether or not your listener should fire if the event has been cancelled before it is the listener's turn to handle the event. False by default.

Example:
```csharp
using Minecraft.Server.FourKit.Event;
using Minecraft.Server.FourKit.Event.Player;

public class MyListener : Listener
{
    // executes before the second method because it has a much lower priority.
    [EventHandler(Priority = EventPriority.Lowest)]
    public void onPlayerChat1(PlayerChatEvent e) {
        event.setCancelled(true);
    }

    // Will not execute unless another listener with a lower priority has uncancelled the event.
    [EventHandler(Priority = EventPriority.Highest, IgnoreCancelled = true)]
    public void onPlayerChat2(PlayerChatEvent e) {
        Console.WriteLine("This should not execute.");
    }
}
```

@section commands Creating Commands

A big thing you will probably want to do is learn how to create commands.

They are not like bukkit, you dont fill out a yml file.

### Creating our Command class

Lets start by creating our actual command handler. You must have a class that extends the \ref Minecraft.Server.FourKit.Command.CommandExecutor "CommandExecutor" class.

```csharp
public class CoolCommand : CommandExecutor
{
    public bool onCommand(CommandSender sender, Command command, string label, string[] args)
    {
        return true;
    }
}
```

`sender` is the actual command sender. This can be either a \ref Minecraft.Server.FourKit.Entity.Player "Player" or a \ref Minecraft.Server.FourKit.Command.ConsoleCommandSender "ConsoleCommandSender"

`command` is the actual command.

`label` is the command name they used to execute.

`args` is the command arguments passed.

You might notice that the `onCommand` func returns a boolean. This indicates if the command executed successfully.

### Registering the command

Now, lets actually register this command. To register the command, you have to use `FourKit.getCommand("command").setExecutor()`

```csharp
public void onEnable()
{
    FourKit.getCommand("cool").setExecutor(new CoolCommand());
}
```

Now we can run the command by running `/cool` in chat or typing `cool` in console!

getCommand() returns a \ref Minecraft.Server.FourKit.Command.PluginCommand "PluginCommand" class. You can see all the functions you can use from here!

Now, when we run `help` in console, we should see this:

```
[2026-03-20 23:31:19.462][INFO][console] Plugin commands:
[2026-03-20 23:31:19.463][INFO][console]   /cool
```

### Defining usage and description for the command

We can even add a description and define usage to the command!

```csharp
FourKit.getCommand("cool").setDescription("my awesome command");
FourKit.getCommand("cool").setUsage("/cool <arg1>");
```

Now it shows this:

```
[2026-03-20 23:38:06.470][INFO][console] Plugin commands:
[2026-03-20 23:38:06.470][INFO][console]   /cool <arg1> - my awesome command
```

### Checking who is running the command

Now that we can do all this, we can check who is running the command. Best way to do this is check if the sender is an instance of \ref Minecraft.Server.FourKit.Entity.Player "Player" or \ref Minecraft.Server.FourKit.Command.ConsoleCommandSender "ConsoleCommandSender".

```csharp
public bool onCommand(CommandSender sender, Command command, string label, string[] args)
{
    if (sender is ConsoleCommandSender)
    {
        // sender is console.
        sender.sendMessage("Whats good console");
        return true;
    }
    // sender is player
    Player p = (Player)sender;
    p.sendMessage("Do it work?");
    return true;
}
```

When console runs this command, they will see "Whats good console" in console. When a Player runs this command, they will see "Do it work?" in chat.

From there on, you can do whatever you want in the command. You can modify the player, such as teleport them somewhere. You can do whatever you want!

@section dependencies Dependencies

Say you want to make a plugin that links a Discord bot to your plugin. This is possible! You can use something like [Discord.NET](https://docs.discordnet.dev/) for that.

When a plugin needs dependencies, you also need to bring over the DLL's for the dependencies.

You can put them into a folder under the plugins folder next to the main plugin dll.

Example folder structure:

- plugins/
- plugins/MyPlugin/MyPlugin.dll
- plugins/MyPlugin/CoolDependency.dll

When a plugin folder is made, make sure the main dll matches the name of the folder, or else it will skip it.

You can also avoid this by using [Fody Costura](https://github.com/Fody/Costura) and bundle the dependencies into your DLL.

### Fody Costura

Fody Costura isnt very well documented, but heres the general usage guide that has worked for users:

<!-- why doesnt doxygen support 1. 2. 3. etc? -->

<ol>
  <li>
    Install Fody Costura
    <p>This can be through NuGet, or through anything you wish to use for getting dependencies.</p>
  </li>

  <li>
    Create a FodyWeavers.xml in your project root:
```xml
<?xml version="1.0" encoding="utf-8"?>
<Weavers xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="FodyWeavers.xsd">
  <Costura>
    <ExcludeAssemblies>
      Minecraft.Server.FourKit
    </ExcludeAssemblies>
  </Costura>
</Weavers>
```
    <p>This will exclude bundling fourkit in the DLL too.</p>
  </li>

  <li>
    Make csproj copy dependency DLL files over to build dir
    <p>You can do this by adding <code>&lt;CopyLocalLockFileAssemblies&gt;true&lt;/CopyLocalLockFileAssemblies&gt;</code> to the property group.</p>
  </li>

  <li>
    Build
    <p>After you've done all this, it should build and put all dependencies into one DLL in your output folder.</p>
  </li>
</ol>