using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server
{
    class GrandientDescender
    {
        public static double[] minimize(Func<Double[],Object,Double> costFunction, int maxIterations, Object parameters, double[] startingPoint,double stepsize)
        {
            double cost;
            double oldcost = Double.MaxValue;
            double[] change = new double[startingPoint.Length];
            double derivative;
            while (maxIterations>0)
            {
                maxIterations--;
                cost = costFunction(startingPoint, parameters);
                if(cost>=oldcost)
                {
                    stepsize /= 2.1;
                }
                else
                {
                    stepsize *= 1.9;
                }
                oldcost = cost;
                for (int i = 0; i < startingPoint.Length; i++)
                {
                    startingPoint[i] += stepsize;
                    derivative = (costFunction(startingPoint, parameters) - cost) / stepsize;
                    startingPoint[i] -= stepsize;
                    change[i] = stepsize * derivative;
                }
                for (int i = 0; i < startingPoint.Length; i++)
                {
                    startingPoint[i] -= change[i];
                }
            }
            return startingPoint;
        }
    }
}
