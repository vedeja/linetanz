var linetanz = new LineTanzSketch();
linetanz.Setup();

Console.WriteLine("Press ESC to exit");

while (true)
{
    linetanz.Loop();

    if (Console.KeyAvailable)
    {
        var k = Console.ReadKey(true);

        switch (k.Key)
        {
            case ConsoleKey.Escape:
                return;

            case ConsoleKey.M:
                linetanz.ToggleMute();                
                break;
        }
    }
}