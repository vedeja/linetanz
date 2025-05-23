-- Declare the protocol
mackie_dl32r = Proto("mackiedl32r", "Mackie DL32R Protocol")

-- Create protocol fields
local f_magic = ProtoField.uint8("mackiedl32r.magic", "Magic", base.HEX)
local f_sequencenumber = ProtoField.uint8("mackiedl32r.sequencenumber", "Sequence Number", base.DEC)
local f_bodylength = ProtoField.uint16("mackiedl32r.bodylength", "Body Length (4-byte chunks)", base.DEC, nil, base.NONE, "Big Endian")
local f_actualbodylength = ProtoField.uint32("mackiedl32r.actualbodylength", "Actual Body Length (bytes)", base.DEC)
local f_messagetype = ProtoField.uint8("mackiedl32r.messagetype", "Message Type", base.HEX, {
    [0x00] = "Request",
    [0x01] = "Response",
    [0x05] = "Error",
    [0x08] = "Broadcast"
})
local f_commandtype = ProtoField.uint8("mackiedl32r.commandtype", "Command Type", base.HEX, {
    [0x01] = "KeepAlive",
    [0x03] = "Handshake",
    [0x04] = "FirmwareInfo",
    [0x06] = "MessageSizeControl",
    [0x07] = "Playback",
    [0x0E] = "Info",
    [0x0F] = "TransportControl",
    [0x13] = "ChannelValues",
    [0x15] = "MeterLayout",
    [0x16] = "MeterControl",
    [0x18] = "ChannelNames",
    [0x95] = "StopRecording"
})
local f_checksum = ProtoField.uint16("mackiedl32r.checksum", "Checksum", base.HEX)
local f_body = ProtoField.bytes("mackiedl32r.body", "Body")
local f_body_byte = ProtoField.uint8("mackiedl32r.body_byte", "Body Byte", base.HEX)
local f_bodychecksum = ProtoField.uint32("mackiedl32r.bodychecksum", "Body Checksum", base.HEX)

-- Add fields to the protocol
mackie_dl32r.fields = { f_magic, f_sequencenumber, f_bodylength, f_actualbodylength, f_messagetype, f_commandtype, f_checksum, f_body, f_body_byte, f_bodychecksum }

local function get_message_length(buffer, offset)    
    local body_length_chunks = buffer(offset, 2):uint()
    local body_length_bytes = body_length_chunks * 4

    local header_length = 8
    local body_checksum_length = (body_length_bytes > 0) and 4 or 0
    local length = header_length + body_length_bytes + body_checksum_length

    return length
end

-- Dissector function
function mackie_dl32r.dissector(buffer, pinfo, tree)

    -- Set the protocol column
    pinfo.cols.protocol = mackie_dl32r.name

    local function dissect_message(buffer, pinfo, tree)
        
        -- Check if the buffer length is sufficient
        if buffer:len() < 8 then                        
            pinfo.cols.info = "Incomplete message, shorter than 8 bytes"
            return
        end
        
        -- Check the magic byte
        local magic_byte = buffer(0, 1):uint()
        if magic_byte ~= 0xAB then
            pinfo.cols.info = "Invalid magic byte"
            return
        end
        
        -- Calculate the header checksum
        local sum = 0
        for i = 0, 5 do
            sum = sum + buffer(i, 1):uint()
        end
        local checksum_calc = 0xFFFF - sum
        
        -- Read the provided header checksum
        local checksum_provided = buffer(6, 2):uint()
        
        -- Calculate body length
        local body_length_chunks = buffer(2, 2):uint()
        local body_length_bytes = body_length_chunks * 4
        
        -- Check if the buffer length is sufficient for the body and body checksum
        if buffer:len() < 8 + body_length_bytes + (body_length_bytes > 0 and 4 or 0) then
            pinfo.cols.info = "Incomplete message, shorter than expected"
            return
        end
        
        -- Create the protocol tree
        local subtree = tree:add(mackie_dl32r, buffer(), "Mackie DL32R Protocol Data")
        
        -- Add fields to the tree
        subtree:add(f_magic, buffer(0, 1))
        subtree:add(f_sequencenumber, buffer(1, 1):uint())
        subtree:add(f_bodylength, buffer(2, 2):uint())
        subtree:add(f_actualbodylength, body_length_bytes)
        subtree:add(f_messagetype, buffer(4, 1):uint())
        subtree:add(f_commandtype, buffer(5, 1):uint())
        subtree:add(f_checksum, buffer(6, 2))
        
        -- Validate header checksum
        if checksum_calc == checksum_provided then
            subtree:add_expert_info(PI_CHECKSUM, PI_NOTE, "Header checksum valid")
        else
            subtree:add_expert_info(PI_CHECKSUM, PI_WARN, string.format("Header checksum invalid (expected 0x%04X)", checksum_calc))
        end
        
        -- Add the body to the tree if present
        if body_length_bytes > 0 then
            local body_subtree = tree:add(mackie_dl32r, buffer(8, body_length_bytes + 4), "DL32R Message Body")
            local body = buffer(8, body_length_bytes)
            body_subtree:add(f_body, body)
            
            -- Calculate the body checksum
            local body_sum = 0
            for i = 0, body_length_bytes - 1 do
                body_sum = body_sum + body(i, 1):uint()
            end
            local body_checksum_calc = 0xFFFFFFFF - body_sum
            
            -- Read the provided body checksum
            local body_checksum_provided = buffer(8 + body_length_bytes, 4):uint()
            
            -- Add the body checksum to the tree
            body_subtree:add(f_bodychecksum, buffer(8 + body_length_bytes, 4))
            
            -- Validate body checksum
            if body_checksum_calc == body_checksum_provided then
                body_subtree:add_expert_info(PI_CHECKSUM, PI_NOTE, "Body checksum valid")
            else
                body_subtree:add_expert_info(PI_CHECKSUM, PI_WARN, string.format("Body checksum invalid (expected 0x%08X)", body_checksum_calc))
            end

        end

        -- Use tcp_dissect_pdus to handle fragmented messages
        tcp_dissect_pdus(buffer, tree, 2, get_message_length, dissect_message)
    end
end

-- Register the dissector
local tcp_port = DissectorTable.get("tcp.port")
tcp_port:add(50001, mackie_dl32r)  -- Use TCP port 50001 for the Mackie DL32R protocol