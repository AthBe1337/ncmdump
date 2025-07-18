import createModule from './wasm/ncmdump.js';

let wasmModule = null;
let isWasmReady = false;

// 初始化 WASM 模块
createModule({
    locateFile: (path) => path.endsWith('.wasm') ? './wasm/ncmdump.wasm' : path,
    onRuntimeInitialized: function() {
        wasmModule = this;
        isWasmReady = true;
        self.postMessage({ type: 'wasm-ready' });

        try {
            wasmModule.FS.mkdir('/work');
        } catch (e) {
            if (e.code !== 'EEXIST') {
                self.postMessage({
                    type: 'error',
                    error: `VFS初始化失败: ${e.message}`
                });
            }
        }
    }
}).catch(err => {
    self.postMessage({
        type: 'error',
        error: `WASM加载失败: ${err.message}`
    });
});

self.onmessage = async (e) => {
    const { type, payload } = e.data;

    if (type === 'decrypt') {
        try {
            if (!isWasmReady) {
                throw new Error("WASM模块尚未初始化完成");
            }

            // 1. 准备输入数据
            const fileData = new Uint8Array(payload.fileData);
            const baseName = payload.baseNameWithoutExtension || "output";

            // 2. 执行解密
            const resultView = wasmModule.decryptNCM(fileData, baseName);

            // 3. 安全转换结果
            const result = new Uint8Array(resultView.length);
            result.set(resultView); // 创建独立拷贝

            // 4. 传输结果（无需手动释放内存）
            self.postMessage({
                type: 'decrypted',
                payload: {
                    index: payload.index,
                    result: result.buffer
                }
            }, [result.buffer]);

        } catch (error) {
            self.postMessage({
                type: 'error',
                payload: {
                    index: payload.index,
                    error: error.message
                }
            });
        }
    }
};
