all: build/Release/poppler.node

build/Release/poppler.node: ./src/*.cc ./src/*.h
	npm install

test: all
	npm test

clean:
	npm run-script clean

debug: all
	npm run-script build-debug
