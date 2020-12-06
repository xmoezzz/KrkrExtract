using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using r2pipe;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace R2
{
    public class StaticAnalysis
    {
        public enum FindStatus
        {
            FOUND_FAKE = 0,
            FOUND_CMP_FLAG0 = 1,
            FOUND_JNZ_ADDR = 2,
            FOUND_MOV_FLAG1 = 3,
            FOUND_RET = 4
        };

        static JObject[] emitCommandArray(ref r2pipe.DllR2Pipe r, string cmd)
        {
            var reply = r.RunCommand(cmd);
            JObject[] parsed = JsonConvert.DeserializeObject<JObject[]>(reply);
            return parsed;
        }

        static JObject emitCommandObject(ref r2pipe.DllR2Pipe r, string cmd)
        {
            var reply = r.RunCommand(cmd);
            return JsonConvert.DeserializeObject<JObject>(reply);
        }

        static void emitCommandVoid(ref r2pipe.DllR2Pipe r, string cmd)
        {
            r.RunCommand(cmd);
        }

        /// <summary>
        /// perform fake static analysis on each function
        /// TODO : convert to SSA form ...
        /// </summary>
        /// <param name="r"></param>
        /// <returns></returns>
        static UInt64 GetPrivateAddressInternal(ref r2pipe.DllR2Pipe r)
        {
            var funcs = emitCommandArray(ref r, "aflj");
            foreach (var func in funcs)
            {
                UInt64 address = (UInt64)func["offset"];
                var nextCmd = string.Format("afbj {0}", address);
                var blocks = emitCommandArray(ref r, nextCmd);

                if (blocks.Length > 5)
                    continue;

                UInt64 size = (UInt64)func["size"];
                if (size <= 0)
                    continue;

                List<JObject> opcodes = new List<JObject>();
                Dictionary<UInt64, JObject> opcodeMap = new Dictionary<ulong, JObject>();

                for (UInt64 offsetInFunc = 0; offsetInFunc < size;)
                {
                    nextCmd = string.Format("s {0}", offsetInFunc + address);
                    emitCommandVoid(ref r, nextCmd);

                    nextCmd = "aoj 1";
                    var fetchedOpcodes = emitCommandArray(ref r, nextCmd);
                    if (fetchedOpcodes.Length == 0)
                        break;

                    var opcode = fetchedOpcodes[0];
                    opcodeMap[offsetInFunc + address] = opcode;
                    opcodes.Add(opcode);
                    offsetInFunc += (UInt64)opcode["size"];
                }

                if (opcodes.Count < 3)
                {
                    Console.WriteLine("opcodes size");
                    continue;
                }

                var opcode0 = opcodes[0];
                if (((string)opcode0["mnemonic"]) != "push")
                {
                    Console.WriteLine("no push");
                    continue;
                }

                var opcode1 = opcodes[1];
                if (((string)opcode1["mnemonic"]) != "cmp")
                {
                    Console.WriteLine("no cmp");
                    continue;
                }

                string opcode1Bytes = (string)opcode1["bytes"];
                if (!opcode1Bytes.StartsWith("803d"))
                {
                    Console.WriteLine("cmp : invalid mode");
                    continue;
                }

                int val = (int)opcode1["val"];
                if (val != 0)
                {
                    Console.WriteLine("cmp : invalid imm");
                    continue;
                }

                UInt64 flagAddr = (UInt64)opcode1["ptr"];
                var status = FindStatus.FOUND_CMP_FLAG0;
                UInt64 funcEndAddr = (UInt64)address + size;

                for (int i = 2; i < opcodes.Count; i++)
                {
                    UInt64 gotoAddr = UInt64.MaxValue;
                    var opcode = opcodes[i];
                    string mnemonic = (string)opcode["mnemonic"];
                    if (mnemonic == "jnz" || mnemonic == "jne")
                    {
                        gotoAddr = (UInt64)opcode["jump"];

                        if (gotoAddr < address || gotoAddr >= funcEndAddr)
                        {
                            Console.WriteLine("invalid addr");
                            break;
                        }

                        status = FindStatus.FOUND_JNZ_ADDR;
                        if (i + 1 > opcodes.Count)
                        {
                            Console.WriteLine("size is too long");
                            break;
                        }

                        i++;

                        opcode = opcodes[i];
                        mnemonic = (string)opcode["mnemonic"];
                        if (mnemonic != "mov")
                        {
                            Console.WriteLine("no mov");
                            break;
                        }

                        string opcodeBytes = (string)opcode["bytes"];
                        if (!opcodeBytes.StartsWith("c605"))
                        {
                            Console.WriteLine("invalid mov");
                            break;
                        }

                        UInt64 opcodePtr = (UInt64)opcode["ptr"];

                        if (opcodePtr != flagAddr || (int)opcode["val"] != 1)
                        {
                            Console.WriteLine("invalid addr or flag");
                            Console.WriteLine("{0} {1}",
                                (opcodePtr != flagAddr).ToString(),
                                ((int)opcode["val"] != 1).ToString()
                            );

                            break;
                        }

                        status = FindStatus.FOUND_MOV_FLAG1;
                        if (!opcodeMap.ContainsKey(gotoAddr))
                        {
                            Console.WriteLine("no mov2");
                            break;
                        }

                        if (i + 1 > opcodes.Count)
                        {
                            Console.WriteLine("size is too long");
                            break;
                        }

                        i++;
                        opcode = opcodes[i];

                        mnemonic = (string)opcode["mnemonic"];
                        if (mnemonic != "call")
                        {
                            Console.WriteLine("no call");
                            break;
                        }

                        status = FindStatus.FOUND_RET;
                        return address;
                    }
                }
            }
            return 0;
        }

        public static UInt64 Run(string FilePath)
        {
            var r = new r2pipe.DllR2Pipe();
            r.RunCommand("o " + FilePath);
            r.RunCommand("aaa");
            var address = GetPrivateAddressInternal(ref r);
            Console.WriteLine(address);
            r.Dispose();
            return address;
        }
    }
}

