CC = cl
CFLAGS = /I"C:/Users/david/Documents/CodingProjects/C_MMO_RPG_rewrite/CJSON_lib/cjson" \
         /I"C:/Users/david/vcpkg/installed/x64-windows/include/mongoc-2.0" \
         /I"C:/Users/david/vcpkg/installed/x64-windows/include/bson-2.0"

LFLAGS = pthreadVC3.lib mongoc2.dll.lib bson2.dll.lib

# Source files
SRCS = Main.c scheduler.c config.c \
       C:/Users/david/Documents/CodingProjects/C_MMO_RPG_rewrite/CJSON_lib/cjson/cJSON.c \
       C:/Users/david/Documents/CodingProjects/C_MMO_RPG_rewrite/LoginRegister/LoginRegister.c \
       C:/Users/david/Documents/CodingProjects/C_MMO_RPG_rewrite/MongoDBReadWriteCache/Cache.c \
       C:/Users/david/Documents/CodingProjects/C_MMO_RPG_rewrite/MongoDBReadWriteCache/ReadUser.c \
       C:/Users/david/Documents/CodingProjects/C_MMO_RPG_rewrite/MongoDBReadWriteCache/UserUtils.c

# Output executable
TARGET = DistributedNodes.exe

# Default rule
all: $(TARGET)

$(TARGET): $(SRCS)
    $(CC) $(SRCS) $(CFLAGS) /link $(LFLAGS) /out:$(TARGET)

clean:
    del *.obj
    del *.exe