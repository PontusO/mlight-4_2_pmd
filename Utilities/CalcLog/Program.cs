using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace ConsoleApplication1
{
    class Program
    {
        static void Main(string[] args)
        {
            double start = -4;
            double end = 6;
            double factor = 65535 / Math.Exp(end);
            int steps = 100;
            double step = (end - start) / (steps - 1);
            int i;
            int value;

            System.IO.StreamWriter file = new System.IO.StreamWriter(@"logvec.h");
            System.IO.StreamWriter file2 = new System.IO.StreamWriter(@"logvec.txt");
                
            file.Write("static const int logvec[] = { 0x0000,");
            for (i = 0; i < 100; i++)
            {
                if ((i % 10) == 0)
                {
                    file.WriteLine();
                    file.Write("\t");
                }
                value = (int)Math.Floor(Math.Exp(start) * factor);
                start += step;
                file2.Write("{0}, ", value);
                file.Write ("0x{0:X4}, ", value);
            }
            file.WriteLine();
            file.WriteLine("};");

            file.Close();
            file2.Close();
        }
    }
}
