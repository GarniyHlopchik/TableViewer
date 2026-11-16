using System;
using Raven;

public static class Program
{
    static uint window;
    static uint help_win;
    static int clicks;

    // OnClicked="hello.clicked" in main.xml fires this action
    static void OnHello(uint sender, IntPtr action, IntPtr userData)
    {
        clicks++;
        // the widget with Name="status"
        Console.WriteLine(clicks);
    }


    public static int Main()
    {
        if (R.Rinit(RBackend.R_BACKEND_VULKAN) == 0) return 1;
        R.Rtheme_apply(RTheme.R_THEME_LIGHT);
        R.Rlocale_load_csv("strings.csv");
        window = R.Rwindow_create_from_xml_file("templates/main.xml");
        help_win = R.Rwindow_create_from_xml_file("templates/help_popup.xml");
        R.Raction_bind("hello.clicked", R.Keep<RActionCallback>(OnHello), IntPtr.Zero);
        while (R.Rdraw_frame() != 0) { }
        R.Rshutdown();
        return 0;
    }
}
