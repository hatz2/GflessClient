class CertificateExtractor {
    private QByteArray_data!: NativeFunction<NativePointer, [NativePointer]>;
    private QIODevice_read!: NativeFunction<number, [NativePointer, NativePointer, number]>;
    private QIODevice_bytesAvailable!: NativeFunction<number, [NativePointer]>;

    constructor() {
        this.initNativeFunctions();
        this.initHook();
    }

    private initNativeFunctions(): void {
        const QByteArray_dataPtr = this.findExportAddressByName("Qt5Core.dll", "?data@QByteArray");
        const QIODevice_readPtr = this.findExportAddressByName("Qt5Core.dll", "?read@QIODevice@@QAE_JPAD_J@Z");
        const QIODevice_bytesAvailablePtr = this.findExportAddressByName("Qt5Core.dll", "?bytesAvailable@QIODevice");

        if (!QByteArray_dataPtr) {
            throw "Could not find address of QByteArray::data";
        }

        if (!QIODevice_readPtr) {
            throw "Could not find address of QIODevice::read";
        }

        if (!QIODevice_bytesAvailablePtr) {
            throw "Could not find address of QIODevice::bytesAvailable";
        }

        this.QByteArray_data = new NativeFunction(QByteArray_dataPtr, 'pointer', ['pointer'], 'thiscall');
        this.QIODevice_read = new NativeFunction(QIODevice_readPtr, 'int', ['pointer', 'pointer', 'int'], 'thiscall');
        this.QIODevice_bytesAvailable = new NativeFunction(QIODevice_bytesAvailablePtr, 'int', ['pointer'], 'thiscall');
    }

    private initHook(): void {
        const importPkcs12Addr = this.findExportAddressByName("Qt5Network.dll", "?importPkcs12@QSslCertificate");

        if (!importPkcs12Addr) {
            throw "Could not find address of QSslCertificate::importPkcs12";
        }

        Interceptor.attach(importPkcs12Addr, { onEnter : (args) => this.onHookEnter(args) });
    }

    private onHookEnter(args: InvocationArguments): void {
        console.log("[*] QSslCertificate::importPkcs12 was called")
        const ioDevice = args[0];
        const passPhrase = this.QByteArray_data(args[4]).readCString();
        const certSize = this.QIODevice_bytesAvailable(ioDevice);
        const cert = Memory.alloc(certSize);
        this.QIODevice_read(ioDevice, cert, certSize);

        send({ 'passPhrase': passPhrase }, cert.readByteArray(certSize));
    }

    private findExportAddressByName(moduleName: string, exportName: string): NativePointer | null | undefined {
        const mod = Process.findModuleByName(moduleName);
        const moduleExportDetails = mod?.enumerateExports();
        let result = null;

        moduleExportDetails?.forEach(module => {
            if (module.name.includes(exportName)) {
                console.log("[+] Function", module.name, "found at", module.address);
                result = module.address;
                return;
            }
        });
        
        return result;
    }
}

const extractor = new CertificateExtractor();