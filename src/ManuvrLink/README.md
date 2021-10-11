Each instance of ManuvrLink represents a persistent dialog with another machine
via some unspecified transport.

The protocol is based on CBOR.



Manuvr's protocol is formatted this way:

All multibyte values are stored "little-endian".
Checksum = 0x55 (Preload) + Every other byte after it. Then truncate to uint8.

|<-------------------HEADER------------------------>|
|--------------------------------------------------|  /---------------\
| Total Length | Checksum | Unique ID | Message ID |  | Optional data |
|--------------------------------------------------|  \---------------/
      3             1           2           2            <---- Bytes

Minimum packet size is the sync packet (4 bytes). Sync packet is
always the same (for a given CHECKSUM_PRELOAD):
0x04 0x00 0x00 0x55
|----- Checksum preload

Sync packets are the way in which we establish initial offset for a dialog over a stream.
We need to sync initially on connection and if something goes sideways in the course
of a normal dialog.

Receiving an unsolicited sync packet should be an indication that the counterparty either
can't understand us, or the transport dropped a message and they timed it out. When we see
a sync packet on the wire, we should allow the currently-active transaction to complete and
then send back a sync stream of our own. When the counterparty notices this, communication
can resume.



## API between firmware instances

### Sequence numbers

###


### Capability discovery

#### Transports

#### Protocols

#### Cryptography

#### Hardware capabilities


### Configuration

### Identity management


### Sending

#### To a specific identity

#### Broadcasting

#### Selecting a protocol and/or transport

#### Console


### Cached messages

### Time and Date

### Location

### Display


----------------------------


## API to Comm unit
