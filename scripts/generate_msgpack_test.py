import msgpack
from io import BytesIO
import binascii


def generate_unsigned():

  for i in list(range(35)) + list(range(100, 141)) + list(range(240, 275)) + list(range( (1 << 16) - 10 , (1<<16) + 10 )) + list(range( (1 << 32) - 10 , (1<<32) + 10 )):
    buf = BytesIO()
    buf.write(msgpack.packb(i))
    ref = binascii.hexlify(buf.getbuffer())

    print("value = %d;" % i)
    print("stream = ByteArrayBuffer();")
    print("stream << value;")
    print("EXPECT_EQ(FromHex(\"%s\"), stream.data());" % ref.decode());
    print("stream.seek(0);");  
    print("value = 0;");    
    print("stream >> value;");
    print("EXPECT_EQ(value, %d);" % i);  
    print("")



for i in list(range(35)) + list(range(100, 141)) + list(range(240, 275)) + list(range( (1 << 16) - 10 , (1<<16) + 10 )) + list(range( (1 << 32) - 10 , (1<<32) + 10 )):
  buf = BytesIO()
  buf.write(msgpack.packb(i))
  ref = binascii.hexlify(buf.getbuffer())

  print("value = %d;" % i)
  print("stream = ByteArrayBuffer();")
  print("stream << value;")
  print("EXPECT_EQ(FromHex(\"%s\"), stream.data());" % ref.decode());
  print("stream.seek(0);");  
  print("value = 0;");    
  print("stream >> value;");
  print("EXPECT_EQ(value, %d);" % i);  
  print("")

  buf = BytesIO()
  buf.write(msgpack.packb(-i))
  ref = binascii.hexlify(buf.getbuffer())

  print("value = %d;" % (-i))
  print("stream = ByteArrayBuffer();")
  print("stream << value;")
  print("EXPECT_EQ(FromHex(\"%s\"), stream.data());" % ref.decode());
  print("stream.seek(0);");  
  print("value = 0;");    
  print("stream >> value;");
  print("EXPECT_EQ(value, %d);" % (-i));
  print("")



#buf = BytesIO()
#buf.write(msgpack.packb({"compact": True, "schema": 0}))
#print(binascii.hexlify(buf.getbuffer()))


#msgpack.unpackb(_, raw=False)


#for i in range(100):
#   buf.write(msgpack.packb(i, use_bin_type=True))

#buf.seek(0)

#unpacker = msgpack.Unpacker(buf, raw=False)
#for unpacked in unpacker:
#    print(unpacked)
