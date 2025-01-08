class Serializer
{
    public static byte[] Serialize(DlMessage message)
    {
        var bodyLength = message.Body.Length;
        var hasBody = bodyLength > 0;
        var bodyChunkCount1 = bodyLength / 4;
        var bodyChunkCount2 = 0; //TODO: handle such large bodies, larger than 255 chunks? Yes, probably - song playback messages appear to have 364 bytes of data in the body
        var headerChecksum = 0xffff - (DlMessage.MESSAGE_START + message.SequenceNumber + bodyChunkCount2 + bodyChunkCount1 + (int)message.Type + (int)message.Command);
        var hcs = BitConverter.GetBytes(headerChecksum);

        var len = 8;
        if (hasBody)
        {
            len += bodyLength;
            len += 4; // space for body checksum
        }

        var data = new byte[len];

        data[0] = DlMessage.MESSAGE_START;
        data[1] = (byte)message.SequenceNumber;
        data[2] = (byte)bodyChunkCount2;
        data[3] = (byte)bodyChunkCount1;
        data[4] = (byte)message.Type;
        data[5] = (byte)message.Command;
        data[6] = hcs[1];
        data[7] = hcs[0];

        if (hasBody)
        {
            var bodyChecksum = 0xffffffff - Enumerable.Sum(message.Body, x => x);
            var bcs = BitConverter.GetBytes(bodyChecksum);

            for (var i = 0; i < bodyLength; i++)
            {
                var b = message.Body[i];
                data[8 + i] = b;
            }

            var bcOffset = 8 + bodyLength;

            data[bcOffset + 0] = bcs[3];
            data[bcOffset + 1] = bcs[2];
            data[bcOffset + 2] = bcs[1];
            data[bcOffset + 3] = bcs[0];
        }

        return data;
    }
}