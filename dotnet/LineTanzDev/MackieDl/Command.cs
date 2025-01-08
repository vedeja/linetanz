public enum Command
{
    /// <summary>
    /// Used as the very first request message, and sent every 2.5 seconds by the client to keep the connection alive.
    /// </summary>
    KeepAlive = 0x01,

    /// <summary>
    /// Sent once from each side early on, to establish basic client and mixer information.
    /// </summary>
    Handshake = 0x03,

    /// <summary>
    /// Sent once from client to mixer, this requests detailed version information.
    /// </summary>
    Version = 0x04,

    /// <summary>
    /// Indicates the size of message the requester is willing to receive.
    /// </summary>
    MessageSizeControl = 0x06,

    /// <summary>
    /// Requests various kinds of general information from the peer.
    /// </summary>
    Info = 0x0E,

    /// <summary>
    /// Song selection, play, stop.
    /// </summary>
    PlaybackTransport = 0x0F,

    /// <summary>
    /// Used to both report channel information (e.g. fader position, muting etc), or indicate a change to that information (e.g. "please mute this channel"). Also used to report meter information.
    /// </summary>
    ChannelValues = 0x13,

    /// <summary>
    /// Client request for the mixer to report meter information periodically.
    /// </summary>
    PeriodicMeters = 0x15,

    /// <summary>
    /// Client request indicating which meter values should be reported.
    /// </summary>
    MeterSelection = 0x16,

    /// <summary>
    /// Used to report channel names.
    /// </summary>
    ChannelNames = 0x18
}