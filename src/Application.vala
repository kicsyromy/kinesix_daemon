public class MyApp : Gtk.Application
{
    public MyApp()
    {
        Object (
            application_id: "com.github.kicsyromy.kinesix",
            flags: ApplicationFlags.FLAGS_NONE
        );
    }

    protected override void activate ()
    {
        var main_window = new Gtk.ApplicationWindow (this);
        main_window.default_height = 300;
        main_window.default_width = 300;
        main_window.title = "Kinesix";
        main_window.show_all ();
    }

    private static void on_swiped(Backend.Interface.SwipeDirection direction, int finger_count)
    {
    }

    private static void on_pinched(Backend.Interface.PinchType type, int finger_count)
    {
    }

    public static int main (string[] args)
    {
        Backend.Interface i = new Backend.Interface(on_swiped, on_pinched);
        var devices = i.get_available_devices();
        stdout.printf("%s", devices[0].path);

        var app = new MyApp ();
        return app.run (args);
    }
}