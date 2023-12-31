.PHONY: build
build: project main.c src/arcane.c src/http.c src/endpoints.c src/map.c src/utilities.c
	gcc -DDEBUG main.c src/arcane.c src/http.c src/endpoints.c src/map.c src/utilities.c src/updater.c src/logger.c -o project/main.exe -lws2_32 -lsqlite3 -lz -lconfuse -lcjson -Wall

