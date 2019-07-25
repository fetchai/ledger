# import msgpack
import umsgpack as msgpack
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


def generate_signed_integer():
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


def short_string():
  TEXT = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum vitae massa a, suscipit pellentesque magna. Cras eget volutpat nibh. Maecenas laoreet interdum felis nec tincidunt. Nulla varius rutrum turpis, eget dignissim risus fringilla non. Duis posuere, leo ullamcorper cursus hendrerit, urna justo molestie arcu, eu tempor turpis nulla quis velit. Ut vel odio venenatis, blandit tortor in, dignissim tortor. Nam in augue eu ipsum hendrerit placerat eget ut sapien. Curabitur nisi lectus, imperdiet sed diam in, interdum aliquam nunc. Praesent nisl mi, venenatis sed facilisis in, tincidunt sed sem. Sed sollicitudin risus eu dui condimentum accumsan. Nulla auctor dictum tortor nec lacinia. Sed gravida lectus orci, eu dapibus tortor blandit nec. Donec iaculis justo eget mauris vulputate, vitae ullamcorper neque iaculis. Pellentesque sed urna ut dolor luctus ultricies. Nam condimentum magna nec felis lobortis, et elementum massa finibus. Phasellus blandit odio orci, ac gravida enim ullamcorper quis. Mauris imperdiet, lorem vitae dignissim efficitur, lorem urna porttitor sem, imperdiet viverra turpis ante ac justo. Mauris maximus porta felis, vitae laoreet sapien semper ac. Proin enim elit, aliquam id purus in, blandit venenatis metus. Nulla risus nibh, ullamcorper vel lectus ac, eleifend accumsan neque. Nunc placerat consequat bibendum. Phasellus turpis risus, maximus vitae turpis in, sollicitudin bibendum est. In nec sapien vel justo suscipit tempus. Integer in ligula condimentum, porttitor elit quis, dignissim velit. Orci varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Cras volutpat neque nulla, non efficitur libero cursus vitae. Suspendisse at lorem vitae leo ultricies laoreet nec quis neque. Nulla augue ipsum, pharetra ac urna ac, blandit blandit risus. Ut leo ligula, cursus nec vestibulum a, accumsan quis sem. Cras elit felis, gravida a imperdiet sit amet, commodo non ante. Sed ligula nisl, molestie et semper nec, aliquam eget velit. Proin ullamcorper arcu eu erat congue tempus. Integer commodo cursus ullamcorper. Praesent nec lectus sed nisi interdum pulvinar. Vivamus ultrices pretium ligula a pulvinar. Mauris in erat ac justo euismod varius ut varius nisl. Nunc ac volutpat massa, eleifend sollicitudin elit. Proin hendrerit accumsan aliquam. Nulla risus lectus, fringilla sed vulputate eu, pellentesque a nisl. Ut vitae velit auctor, iaculis leo eget, facilisis orci. Proin nec consequat arcu. Duis condimentum dolor sed felis sagittis, sit amet scelerisque erat tincidunt. Nam egestas magna augue, eu sodales augue fringilla id. Nam viverra elementum pulvinar. Fusce mi enim, tristique eu lorem accumsan, sagittis mattis enim. Mauris vulputate vitae purus id egestas. Donec erat odio, maximus ut placerat in, cursus id leo. Nunc posuere lobortis ante, ut sagittis est pulvinar eget. Orci varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Nulla pulvinar commodo lectus. Nam auctor urna sed ante consectetur dapibus. Donec tempor ultricies magna feugiat facilisis. Nunc venenatis orci tortor, quis blandit velit euismod sit amet. Fusce aliquet sapien eu arcu egestas mattis. Fusce id eros et leo mattis dignissim eu vitae sapien. Nunc auctor felis sed lorem laoreet, ac pharetra erat lobortis. Mauris a sem sem. Donec gravida quis ex non semper. Praesent at libero vel mi porttitor efficitur quis ut metus. Integer sodales ante tortor, sit amet ullamcorper nunc laoreet lobortis. In aliquet tortor a velit facilisis, id varius lacus ultrices. Fusce at massa odio."

  for i in list(range(35)) + list(range(100, 141)) + list(range(240, 275)) :
    buf = BytesIO()
    str = TEXT[:i]
    buf.write(msgpack.packb(str))
    ref = binascii.hexlify(buf.getbuffer())

    print("// len(value) =", len(str))
    print("value = \"%s\";" % str)
    print("stream = ByteArrayBuffer();")
    print("stream << value;")
    print("EXPECT_EQ(FromHex(\"%s\"), stream.data());" % ref.decode());
    print("stream.seek(0);");  
    print("value = \"\";");    
    print("stream >> value;");
    print("EXPECT_EQ(value, \"%s\");" % str);  
    print("")





TEXT = []
for j in range((1<<16) + 10):
  TEXT.append( chr(ord('a')+(j%26)) )
TEXT = "".join(TEXT)

for i in list(range( (1 << 16) - 10 , (1<<16) + 10 )): # + list(range( (1 << 32) - 10 , (1<<32) + 10 )):
  buf = BytesIO()
  str = TEXT[:i]
  buf.write(msgpack.packb(str))
  ref = binascii.hexlify(buf.getbuffer())

  print("// len(value) =", i)
  print("value = text_buffer.SubArray(0, %d); " % i)
  print("stream = ByteArrayBuffer();")
  print("stream << value;")
  print("EXPECT_EQ(FromHex(\"%s\"), stream.data().SubArray(0,32));" % ref.decode()[:64]);
  print("stream.seek(0);");  
  print("value2 = \"\";");    
  print("stream >> value2;");
  print("EXPECT_EQ(value, value2);");  
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
