using Nefarius.Drivers.HidHide;
using Nefarius.Utilities.DeviceManagement.PnP;
using System.Runtime.InteropServices;

public static class NativeMethods {

    // Declares managed prototypes for unmanaged functions.
    [DllImport("User32.dll", EntryPoint = "MessageBox",
        CharSet = CharSet.Auto)]
    internal static extern int MsgBox(
        IntPtr hWnd, string lpText, string lpCaption, uint uType);

    // Causes incorrect output in the message window.
    [DllImport("User32.dll", EntryPoint = "MessageBoxW",
        CharSet = CharSet.Ansi)]
    internal static extern int MsgBox2(
        IntPtr hWnd, string lpText, string lpCaption, uint uType);

    // Causes an exception to be thrown. EntryPoint, CharSet, and
    // ExactSpelling fields are mismatched.
    [DllImport("User32.dll", EntryPoint = "MessageBox",
        CharSet = CharSet.Ansi, ExactSpelling = true)]
    internal static extern int MsgBox3(
        IntPtr hWnd, string lpText, string lpCaption, uint uType);
}

public class Program {

    private static void Main(string[] args) {
        if (args.Length != 3) {
            NativeMethods.MsgBox(0, "Not enough arguments!", "Error", 0);
            return;
        }

        HidHideControlService hidHide = new HidHideControlService();

        if (!hidHide.IsInstalled) {
            Environment.Exit(0);
            return;
        }

        if (!File.Exists(args[2])) {
            NativeMethods.MsgBox(0, $"Couldn't DualSenseY!", "Error", 0);
            return;
        }

        hidHide.AddApplicationPath(args[2]);

        string instanceID = PnPDevice.GetInstanceIdFromInterfaceId(args[0]);
        if (args[1] == "hide") {
            hidHide.AddBlockedInstanceId(instanceID);
            hidHide.IsAppListInverted = false;
            hidHide.IsActive = true;
        }
        else if (args[1] == "show") {
            hidHide.RemoveBlockedInstanceId(instanceID);
            hidHide.IsActive = false;
        }

        PnPDevice Device = PnPDevice.GetDeviceByInstanceId(instanceID);
        try {
            Device.Disable();
        }
        catch { }
        Device.Enable();
    }
}