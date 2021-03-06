#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mex.h>

#define pow(x,y) ((x)*(x))

void VCalc(int *riskSetInd, int *riskSet, int *nDeath, int *nrow, int *ncol, double *eta, double *V)
{
    int i, j;
    int ndeath = nDeath[0];
    double* C = NULL;
    C = malloc(sizeof(double) * ndeath);
    int n = nrow[0];
    double* expEta = NULL;
    expEta = malloc(sizeof(double) * n);
    
    for(i = 0; i < n; i++)
    {
        expEta[i] = exp(eta[i]);
    }
    
    for(i = 0; i < ndeath; i++)
    {
        C[i] = 0;
        for(j = (riskSetInd[i]-1); j < (riskSetInd[i+1]-1); j++)
        {
            C[i] = C[i] + expEta[j];
        }
    }
    
    
    V[ndeath - 1] = C[ndeath - 1];
    
    for(i = 2; i <= ndeath; i++)
    {
        V[ndeath - i] = V[ndeath - i + 1] + C[ndeath - i];
    }
    free(C);
    free(expEta);
}



void gradCalc(int *riskSetInd, int *riskSet, int *numDeath, int *status, int *nDeath, int *nrow, int *ncol, double *eta, double *ldot, double *V)
{
    int i, j, k;
    int ndeath = nDeath[0];
    double* C = NULL;
	C = malloc(sizeof(double) * ndeath);
    double w;
    int n = nrow[0];
    double* expEta = NULL;
	expEta = malloc(sizeof(double) * n);
    
    for(i = 0; i < n; i++)
    {
        expEta[i] = exp(eta[i]);
    }
    
    for(i = 0; i < ndeath; i++)
    {
        C[i] = 0;
        for(j = (riskSetInd[i]-1); j < (riskSetInd[i+1]-1); j++)
        {
            C[i] = C[i] + expEta[j];
        }
    }
    
    
    V[ndeath - 1] = C[ndeath - 1];
    
    for(i = 2; i <= ndeath; i++)
    {
        V[ndeath - i] = V[ndeath - i + 1] + C[ndeath - i];
    }
    
    for(k = 0; k < n; k++)
    {
        w = 0;
        for(i = 0; i < riskSet[k]; i++)
        {
            w = w + expEta[k] * numDeath[i]/V[i];
        }
        ldot[k] = -(status[k] - w)/nrow[0];    /* OR MAYBE NOT?? */
    }
    
    free(C);
    free(expEta);
}



double negLogLikelihoodCalc(int *riskSetInd, int *riskSet, int *numDeath, int *nDeath, int *nrow, int *ncol, double *eta, double* V, int *deathInd, int *totDeath)
{
    int i;
    double first = 0;
    double second = 0;
    for(i = 0; i < totDeath[0]; i++)
    {
        first = first + eta[deathInd[i]-1];
    }
    
    
    for(i = 0; i < nDeath[0]; i++)
    {
        second = second + numDeath[i] * log(V[i]);
    }
    return (-first + second)/nrow[0];  /* OR MAYBE NOT? */
}



void Solver(double *X, int* index, int *nrow, int *ncol, int *numGroup, double *beta, int *rangeGroupInd, int *groupLen, double *lambda1, double *lambda2, int *innerIter, double *thresh, double *ldot, double *nullBeta, double *gamma, double *V, int *deathInd, int *totDeath, int *riskSetInd, int *riskSet, int *numDeath, int *status, int *nDeath, double *eta, int* betaIsZero, int* groupChange, int* isActive, int* useGroup, double *step)
{
    int i, j, k;
    double *theta = malloc(sizeof(double) * ncol[0]);
    double *thetaNew = malloc(sizeof(double) * ncol[0]);
    int startInd = 0;
    double zeroCheck = 0;
    double check = 0;
    int count = 0;
    double t = step[0];
    double diff = 1;
    double norm = 0;
    double uOp = 0;
    double Lnew = 0;
    double Lold = 0;
    double sqNormG = 0;
    double iProd = 0;
    double *etaNew = NULL;
    etaNew = malloc(sizeof(double) * nrow[0]);
    double *etaNull = NULL;
    etaNull = malloc(sizeof(double) * nrow[0]);
    int reset = 20;
    
    for(i = 0; i < numGroup[0]; i++)
    {
        if(useGroup[i] == 1)
        {
            startInd = rangeGroupInd[i];
            
            
            for(k = 0; k < nrow[0]; k++)
            {
                etaNull[k] = eta[k];
                for(j = startInd; j < rangeGroupInd[i] + groupLen[i]; j++)
                {
                    etaNull[k] = etaNull[k] - X[k + nrow[0] * j] * beta[j];
                }
            }
            
     
            gradCalc(riskSetInd, riskSet, numDeath, status, nDeath, nrow, ncol, etaNull, ldot, V);
            
            double *grad = NULL;
            grad = malloc(sizeof(double) * groupLen[i]);
            
            for(j = 0; j < groupLen[i]; j++)
            {
                grad[j] = 0;
                for(k = 0; k < nrow[0]; k++)
                {
                    grad[j] = grad[j] + X[k + nrow[0] * (j + rangeGroupInd[i])] * ldot[k];
                }
                if(grad[j] < lambda1[0] && grad[j] > -lambda1[0])
                {
                    grad[j] = 0;
                }
                if(grad[j] > lambda1[0])
                {
                    grad[j] = grad[j] - lambda1[0];
                }
                if(grad[j] < -lambda1[0])
                {
                    grad[j] = grad[j] + lambda1[0];
                }
                if(pow(grad[j],2) == pow(lambda1[0],2))
                {
                    grad[j] = 0;
                }
                
            }
            
            zeroCheck = 0;
            for(j = 0; j < groupLen[i]; j++)
            {
                zeroCheck = zeroCheck + pow(grad[j],2);
            }
            
            if(zeroCheck <= pow(lambda2[0],2)* groupLen[i])  
            {
                if(betaIsZero[i] == 0)
                {
                    for(k = 0; k < nrow[0]; k++)
                    {
                        for(j = rangeGroupInd[i]; j < rangeGroupInd[i] + groupLen[i]; j++)
                        {
                            eta[k] = eta[k] - X[k + nrow[0] * j] * beta[j];
                        }
                    }
                }
                betaIsZero[i] = 1;
                for(j = 0; j < groupLen[i]; j++)
                {
                    beta[j + rangeGroupInd[i]] = 0;
                }
            }
            else
            {
                if(isActive[i] == 0)
                {
                    *groupChange = 1;
                }
                isActive[i] = 1;
                
                for(k = 0; k < ncol[0]; k++)
                {
                    theta[k] = beta[k];
                }
                
                betaIsZero[i] = 0;
                double *z = NULL;
                z = malloc(sizeof(double) * groupLen[i]);
                double *U = NULL;
                U = malloc(sizeof(double) * groupLen[i]);
                double *G = NULL;
                G = malloc(sizeof(double) * groupLen[i]);
                double *betaNew = NULL;
                betaNew = malloc(sizeof(double) * ncol[0]);
                
                count = 0;
                check = 1;
                
                while(count <= innerIter[0] && check > thresh[0])
                {
                    
                    count++;
                    
                    gradCalc(riskSetInd, riskSet, numDeath, status, nDeath, nrow, ncol, eta, ldot, V);
                    
                    for(j = 0; j < groupLen[i]; j++)
                    {
                        grad[j] = 0;
                        for(k = 0; k < nrow[0]; k++)
                        {
                            grad[j] = grad[j] + X[k + nrow[0] * (j + rangeGroupInd[i])] * ldot[k];
                        }
                        
                    }
                    
                    diff = -1;
                
                    
                    Lold = negLogLikelihoodCalc(riskSetInd, riskSet, numDeath, nDeath, nrow, ncol, eta, V, deathInd, totDeath);
                    
                 
                    
                    while(diff < 0)
                    {
                        for(j = 0; j < groupLen[i]; j++)
                        {
                            z[j] = beta[j + rangeGroupInd[i]] - t * grad[j];
                            if(z[j] < lambda1[0] * t && z[j] > -lambda1[0] * t)
                            {
                                z[j] = 0;
                            }
                            if(z[j] >= lambda1[0] * t)
                            {
                                z[j] = z[j] - lambda1[0] * t;
                            }
                            if(z[j] <= -lambda1[0] * t)
                            {
                                z[j] = z[j] + lambda1[0] * t;
                            }
                        }
                        
                        norm = 0;
                        for(j = 0; j < groupLen[i]; j++)
                        {
                            norm = norm + pow(z[j],2);
                        }
                        norm = sqrt(norm);
                        if(norm != 0){
                            uOp = (1 - lambda2[0]*sqrt((double)groupLen[i])*t/norm);   
                        }
                        else{uOp = 0;}
                        
                        if(uOp < 0)
                        {
                            uOp = 0;
                        }
                        
                        for(j = 0; j < groupLen[i]; j++)
                        {
                            U[j] = uOp*z[j];
                            G[j] = 1/t *(beta[j + rangeGroupInd[i]] - U[j]);
                            
                        }
                        
                       
                        
                        for(j = 0; j < rangeGroupInd[i]; j++)
                        {
                            thetaNew[j] = beta[j];
                        }
                        for(j = rangeGroupInd[i]; j < rangeGroupInd[i] + groupLen[i]; j++)
                        {
                            thetaNew[j] = beta[j] - t * G[j];
                        }
                        
                        for(j = rangeGroupInd[i] + groupLen[i]; j < ncol[0]; j++)
                        {
                            thetaNew[j] = beta[j];
                        }
                        for(k = 0; k < nrow[0]; k++)
                        {
                            etaNew[k] = eta[k];
                            for(j = 0; j < groupLen[i]; j++)
                            {
                                etaNew[k] = etaNew[k] - t*G[j] * X[k + nrow[0]*(rangeGroupInd[i] + j)];
                            }
                        }
                        
                        VCalc(riskSetInd, riskSet, nDeath, nrow, ncol, etaNew, V);
                        
                        Lnew = negLogLikelihoodCalc(riskSetInd, riskSet, numDeath, nDeath, nrow, ncol, etaNew, V, deathInd, totDeath);
                        
                        sqNormG = 0;
                        iProd = 0;
                        
                        for(j = 0; j < groupLen[i]; j++)
                        {
                            sqNormG = sqNormG + pow(G[j],2);
                            iProd = iProd + grad[j] * G[j];
                        }
                        
                        diff = Lold - Lnew - t * iProd + t/2 * sqNormG;
                        
                        t = t * gamma[0];
                    }
                    t = t / gamma[0];
                    
                    check = 0;
                    
                    for(j = 0; j < groupLen[i]; j++)
                    {
                        check = check + fabs(theta[j + rangeGroupInd[i]] - U[j]);
                        for(k = 0; k < nrow[0]; k++)
                        {
                            eta[k] = eta[k] - X[k + nrow[0] * (j + rangeGroupInd[i])]*beta[j + rangeGroupInd[i]];
                        }
                        beta[j + rangeGroupInd[i]] = U[j] + count%reset/(count%reset+3) * (U[j] - theta[j + rangeGroupInd[i]]);
                        theta[j + rangeGroupInd[i]] = U[j];
                        
                        for(k = 0; k < nrow[0]; k++)
                        {
                            eta[k] = eta[k] + X[k + nrow[0] * (j + rangeGroupInd[i])]*beta[j + rangeGroupInd[i]];
                        }
                    }
                }
                free(z);
                free(U);
                free(G);
                free(betaNew);
            }
            free(grad);
        }
    }
    free(etaNew);
    free(etaNull);
    free(theta);
    free(thetaNew);
}



int coxSolver(double *X, int* index, int *nrow, int *ncol, int *numGroup, int *rangeGroupInd, int *groupLen, double *lambda1, double *lambda2, double *beta, int *innerIter, int *outerIter, double *thresh, double *outerThresh, int *riskSetInd, int *riskSet, int *numDeath, int *status, int *nDeath, double *eta, double *gamma, int *deathInd, int *totDeath, int *betaIsZero, double *step)
{
    int i;
    double* nullBeta = NULL;
    nullBeta = malloc(sizeof(double) * ncol[0]);
    double* V = NULL;
    V = malloc(sizeof(double) * nDeath[0]);
    int n = nrow[0];
    int p = ncol[0];
    double *ldot = NULL;
    ldot = malloc(sizeof(double) * n);
    int groupChange = 1;
    int* isActive = NULL;
    isActive = malloc(sizeof(int) * numGroup[0]);
    int* useGroup = NULL;
    useGroup = malloc(sizeof(int) * numGroup[0]);
    int* tempIsActive = NULL;
    tempIsActive = malloc(sizeof(int) * numGroup[0]);
    
    for(i = 0; i < numGroup[0]; i++)
    {
        isActive[i] = 0;
        useGroup[i] = 1;
    }
    
    
    int outermostCounter = 0;
    double outermostCheck = 100;
    double* outerOldBeta = NULL;
    outerOldBeta = malloc(sizeof(double) * p);
    
    while(groupChange == 1)
    {
        groupChange = 0;
        
        Solver(X, index, nrow, ncol, numGroup, beta, rangeGroupInd, groupLen, lambda1, lambda2, innerIter, thresh, ldot, nullBeta, gamma, V, deathInd, totDeath, riskSetInd, riskSet, numDeath, status, nDeath, eta, betaIsZero, &groupChange, isActive, useGroup, step);
        
        while(outermostCounter < outerIter[0] && outermostCheck > outerThresh[0])
        {
            outermostCounter ++;
            for(i = 0; i < p; i++)
            {
                outerOldBeta[i] = beta[i];
            }
            
            for(i = 0; i < numGroup[0]; i++)
            {
                tempIsActive[i] = isActive[i];
            }
            
            Solver(X, index, nrow, ncol, numGroup, beta, rangeGroupInd, groupLen, lambda1, lambda2, innerIter, thresh, ldot, nullBeta, gamma, V, deathInd, totDeath, riskSetInd, riskSet, numDeath, status, nDeath, eta, betaIsZero, &groupChange, isActive, tempIsActive, step);
            
            outermostCheck = 0;
            for(i = 0; i < p; i++)
            {
                outermostCheck = outermostCheck + fabs(outerOldBeta[i] - beta[i]);
            }
        }
    }
    
    free(V);
    free(nullBeta);
    free(outerOldBeta);
    free(ldot);
    free(useGroup);
    free(isActive);
    free(tempIsActive);
    
    return 1;
}

void Cox(int *riskSetInd, int *riskSet, int *numDeath, int *status, int *nDeath, int *nrow, int *ncol, double *beta, double *eta, double *y, double *weights)
{
    int i, j, k;
    int ndeath = nDeath[0];
    double* C = NULL;
    C = malloc(sizeof(double)*ndeath);
    double* V = NULL;
    V = malloc(sizeof(double)*ndeath);
    double w;
    double r;
    int n = nrow[0];
    double* expEta = NULL;
    expEta = malloc(sizeof(double)*n);
    double* ldot = NULL;
    ldot = malloc(sizeof(double)*n);
    
    for(i = 0; i < n; i++)
    {
        expEta[i] = exp(eta[i]);
    }
    
    for(i = 0; i < ndeath; i++)
    {
        C[i] = 0;
        for(j = (riskSetInd[i]-1); j < (riskSetInd[i+1]-1); j++)
        {
            C[i] = C[i] + expEta[j];
        }
    }
    
    
    
    V[ndeath - 1] = C[ndeath - 1];
    
    for(i = 2; i <= ndeath; i++)
    {
        V[ndeath - i] = V[ndeath - i + 1] + C[ndeath - i];
    }
    
    for(k = 0; k < n; k++)
    {
        w = 0;
        r = 0;
        for(i = 0; i < riskSet[k]; i++)
        {
            w = w + expEta[k] * numDeath[i]/V[i];
            r = r + (expEta[k] * V[i] - pow(expEta[k],2)*numDeath[i])/(pow(V[i],2));
        }
        weights[k] = r;
        ldot[k] = -(status[k] - w);
    }
    
    for(i = 0; i < n; i++)
    {
        y[i] = eta[i] - ldot[i]/weights[i];
    }
    free(C);
    free(V);
    free(ldot);
    free(expEta);
}






void linGradCalc(int *nrow, double *eta, double *y, double *ldot)
{
    int i;
    for(i = 0; i < nrow[0]; i++)
    {
        ldot[i] = (eta[i] - y[i])/nrow[0];  /* OR MAYBE NOT? */
    }
}


double linNegLogLikelihoodCalc(int *nrow, double *eta, double *y)
{
    int i;
    double squareSum = 0;
    
    for(i = 0; i < nrow[0]; i++)
    {
        squareSum = squareSum + pow(eta[i] - y[i], 2)/2;
    }
    
    return squareSum/nrow[0];   /* OR MAYBE NOT? */
}

void linSolver(double *X, double *y, int* index, int *nrow, int *ncol, int *numGroup, double *beta, int *rangeGroupInd, int *groupLen, double *lambda1, double *lambda2, int *innerIter, double *thresh, double *ldot, double *nullBeta, double *gamma, double *eta, int* betaIsZero, int* groupChange, int* isActive, int* useGroup, double *step, int *reset)
{
    int i, j, k;
    double *theta = malloc(sizeof(double)*ncol[0]);
    int startInd = 0;
    double zeroCheck = 0;
    double check = 0;
    int count = 0;
    double t = step[0];
    double diff = 1;
    double norm = 0;
    double uOp = 0;
    double Lnew = 0;
    double Lold = 0;
    double sqNormG = 0;
    double iProd = 0;
    double *etaNew = NULL;
    etaNew = malloc(sizeof(double)*nrow[0]);
    double *etaNull = NULL;
    etaNull = malloc(sizeof(double)*nrow[0]);
   
	
    for(i = 0; i < numGroup[0]; i++)
    {
        if(useGroup[i] == 1)
        {
            startInd = rangeGroupInd[i];
            
            
            for(k = 0; k < nrow[0]; k++)
            {
                etaNull[k] = eta[k];
                for(j = startInd; j < rangeGroupInd[i] + groupLen[i]; j++)
                {
                    etaNull[k] = etaNull[k] - X[k + nrow[0] * j] * beta[j];
                }
            }
            
           
            linGradCalc(nrow, etaNull, y, ldot);
            
            double *grad = NULL;
            grad = malloc(sizeof(double)*groupLen[i]);
            
            for(j = 0; j < groupLen[i]; j++)
            {
                grad[j] = 0;
                for(k = 0; k < nrow[0]; k++)
                {
                    grad[j] = grad[j] + X[k + nrow[0] * (j + rangeGroupInd[i])] * ldot[k];
                }
                if(grad[j] < lambda1[0] && grad[j] > -lambda1[0])
                {
                    grad[j] = 0;
                }
                if(grad[j] > lambda1[0])
                {
                    grad[j] = grad[j] - lambda1[0];
                }
                if(grad[j] < -lambda1[0])
                {
                    grad[j] = grad[j] + lambda1[0];
                }
                if(pow(grad[j],2) == pow(lambda1[0],2))
                {
                    grad[j] = 0;
                }
                
            }
            
            zeroCheck = 0;
            for(j = 0; j < groupLen[i]; j++)
            {
                zeroCheck = zeroCheck + pow(grad[j],2);
            }
            
            if(zeroCheck <= pow(lambda2[0],2)*groupLen[i]) 
            {
                if(betaIsZero[i] == 0)
                {
                    for(k = 0; k < nrow[0]; k++)
                    {
                        for(j = rangeGroupInd[i]; j < rangeGroupInd[i] + groupLen[i]; j++)
                        {
                            eta[k] = eta[k] - X[k + nrow[0] * j] * beta[j];
                        }
                    }
                }
                betaIsZero[i] = 1;
                for(j = 0; j < groupLen[i]; j++)
                {
                    beta[j + rangeGroupInd[i]] = 0;
                }
            }
            else
            {
                if(isActive[i] == 0)
                {
                    *groupChange = 1;
                }
                isActive[i] = 1;
                
                for(k = 0; k < ncol[0]; k++)
                {
                    theta[k] = beta[k];
                }
                
                betaIsZero[i] = 0;
                double *z = NULL;
                z = malloc(sizeof(double)*groupLen[i]);
                double *U = NULL;
                U = malloc(sizeof(double)*groupLen[i]);
                double *G = NULL;
                G = malloc(sizeof(double)*groupLen[i]);
                double *betaNew = NULL;
                betaNew = malloc(sizeof(double)*ncol[0]);
                
                count = 0;
                check = 100000;
                
                
                
                while(count <= innerIter[0] && check > thresh[0])
                {
                    
                    count++;
                    
                    linGradCalc(nrow, eta, y ,ldot);
                    
                    for(j = 0; j < groupLen[i]; j++)
                    {
                        grad[j] = 0;
                        for(k = 0; k < nrow[0]; k++)
                        {
                            grad[j] = grad[j] + X[k + nrow[0] * (j + rangeGroupInd[i])] * ldot[k];
                        }
                        
                    }
                    
                    diff = -1;
                   
                    Lold = linNegLogLikelihoodCalc(nrow, eta, y);
                    
                 
                    
                    while(diff < 0)
                    {
                        for(j = 0; j < groupLen[i]; j++)
                        {
                            z[j] = beta[j + rangeGroupInd[i]] - t * grad[j];
                            if(z[j] < lambda1[0] * t && z[j] > -lambda1[0] * t)
                            {
                                z[j] = 0;
                            }
                            if(z[j] > lambda1[0] * t)
                            {
                                z[j] = z[j] - lambda1[0] * t;
                            }
                            if(z[j] < -lambda1[0] * t)
                            {
                                z[j] = z[j] + lambda1[0] * t;
                            }
                        }
                        
                        norm = 0;
                        for(j = 0; j < groupLen[i]; j++)
                        {
                            norm = norm + pow(z[j],2);
                        }
                        norm = sqrt(norm);

						

                        if(norm != 0){
                            uOp = (1 - lambda2[0]*sqrt((double)groupLen[i])*t/norm);  
                        }
                        else{uOp = 0;}
                        
                        if(uOp < 0)
                        {
                            uOp = 0;
                        }

						

                        for(j = 0; j < groupLen[i]; j++)
                        {
                            U[j] = uOp*z[j];
                            G[j] = 1/t *(beta[j + rangeGroupInd[i]] - U[j]);
                            
                        }
                        
                
                        
                        for(k = 0; k < nrow[0]; k++)
                        {
                            etaNew[k] = eta[k];
                            for(j = 0; j < groupLen[i]; j++)
                            {
                                etaNew[k] = etaNew[k] - t*G[j] * X[k + nrow[0]*(rangeGroupInd[i] + j)];
                            }
                        }
                        
                        Lnew = linNegLogLikelihoodCalc(nrow, etaNew, y);
                        
                        sqNormG = 0;
                        iProd = 0;
                        
                        for(j = 0; j < groupLen[i]; j++)
                        {
                            sqNormG = sqNormG + pow(G[j],2);
                            iProd = iProd + grad[j] * G[j];
                        }
                        
                        diff = Lold - Lnew - t * iProd + t/2 * sqNormG;
                        
                        t = t * gamma[0];
                    }
                    t = t / gamma[0];
                    
					

                    check = 0;
                    
                    for(j = 0; j < groupLen[i]; j++)
                    {
                        check = check + fabs(theta[j + rangeGroupInd[i]] - U[j]);
                        for(k = 0; k < nrow[0]; k++)
                        {
                            eta[k] = eta[k] - X[k + nrow[0] * (j + rangeGroupInd[i])]*beta[j + rangeGroupInd[i]];
                        }
					

                        beta[j + rangeGroupInd[i]] = U[j] + count%reset[0]/(count%reset[0]+3) * (U[j] - theta[j + rangeGroupInd[i]]);
                        theta[j + rangeGroupInd[i]] = U[j];
                        
                        for(k = 0; k < nrow[0]; k++)
                        {
                            eta[k] = eta[k] + X[k + nrow[0] * (j + rangeGroupInd[i])]*beta[j + rangeGroupInd[i]];
                        }
                    }
                }

			

                free(z);
                free(U);
                free(G);
                free(betaNew);
            }
            free(grad);
        }
    }
    free(etaNew);
    free(etaNull);
    free(theta);
}

int linNest(double *X, double* y, int* index, int *nrow, int *ncol, int *numGroup, int *rangeGroupInd, 
	int *groupLen, double *lambda1, double *lambda2, double *beta, int *innerIter, int *outerIter, double *thresh, 
	double *outerThresh, double *eta, double *gamma, int *betaIsZero, double *step, int *reset)
{
    int i, j, k;
    double* prob = NULL;
    prob = malloc(sizeof(double)*nrow[0]);
    double* nullBeta = NULL;
    nullBeta = malloc(sizeof(double)*ncol[0]);
    int n = nrow[0];
    int p = ncol[0];
    double *ldot = NULL;
    ldot = malloc(sizeof(double)*n);
    int groupChange = 1;
    int* isActive = NULL;
    isActive = malloc(sizeof(int)*numGroup[0]);
    int* useGroup = NULL;
    useGroup = malloc(sizeof(int)*numGroup[0]);
    int* tempIsActive = NULL;
    tempIsActive = malloc(sizeof(int)*numGroup[0]);
    
    for(i = 0; i < numGroup[0]; i++)
    {
        isActive[i] = 0;
        useGroup[i] = 1;
    }
	    
  
    int outermostCounter = 0;
    double outermostCheck = 100000;
    double* outerOldBeta = NULL;
    outerOldBeta = malloc(sizeof(double)*p);
	
    while(groupChange == 1)
    {
        groupChange = 0;
        
        linSolver(X, y, index, nrow, ncol, numGroup, beta, rangeGroupInd, groupLen, lambda1, lambda2, innerIter, thresh, ldot, nullBeta, gamma, eta, betaIsZero, &groupChange, isActive, useGroup, step, reset);
        
        while(outermostCounter < outerIter[0] && outermostCheck > outerThresh[0])
        {
            outermostCounter ++;
            for(i = 0; i < p; i++)
            {
                outerOldBeta[i] = beta[i];
            }
            
            for(i = 0; i < numGroup[0]; i++)
            {
                tempIsActive[i] = isActive[i];
            }
            
            linSolver(X, y, index, nrow, ncol, numGroup, beta, rangeGroupInd, groupLen, lambda1, lambda2, innerIter, thresh, ldot, nullBeta, gamma, eta, betaIsZero, &groupChange, isActive, tempIsActive, step, reset);
            
            outermostCheck = 0;
            for(i = 0; i < p; i++)
            {
                outermostCheck = outermostCheck + fabs(outerOldBeta[i] - beta[i]);
            }
        }
    }
    
    free(nullBeta);
    free(outerOldBeta);
    free(ldot);
    free(isActive);
    free(useGroup);
    free(tempIsActive);
    free(prob);

	return 1;
}



void logitGradCalc(int *nrow, double *prob, int *y, double *ldot)
{
    int i;
    for(i = 0; i < nrow[0]; i++)
    {
        ldot[i] = (-y[i] + prob[i])/nrow[0]; /* OR MAYBE NOT? */
    }
}

void pCalc(int *nrow, double *eta, double *prob)
{
    int i;
    for(i = 0; i < nrow[0]; i++)
    {
        prob[i] = exp(eta[i]) / (1 + exp(eta[i]));
    }
}


double logitNegLogLikelihoodCalc(int *nrow, double *prob, int *y)
{
    int i;
    double logLik = 0;
    
    for(i = 0; i < nrow[0]; i++)
    {
        logLik = logLik + y[i] * log(prob[i]) + (1 - y[i]) * log(1 - prob[i]);
    }
    
    return -logLik/nrow[0];  /* OR MAYBE NOT? */
}

void betaZeroSolve(int *nrow, double *betaZero, double *eta, double *prob, double *thresh, int *innerIter, int *y)
{
    int i;
    double diff = 10;
    double num = 0;
    double denom = 0;
    int count = 0;
    
    while(pow(diff,2) > pow(thresh[0],2) && count < innerIter[0])
    {
        pCalc(nrow, eta, prob);
        diff = 0;
        
        for(i = 0; i < nrow[0]; i++)
        {
            num = num + y[i] - prob[i];
            denom = denom + prob[i] * (1 - prob[i]);
        }
        diff = num/denom;
        betaZero[0] = betaZero[0] - diff;
        
        for(i = 0; i < nrow[0]; i++)
        {
            eta[i] = eta[i] + diff;
        }
    }
}


void logitSolver(double *X, int *y, int* index, int *nrow, int *ncol, int *numGroup, double *beta, int *rangeGroupInd, int *groupLen, double *lambda1, double *lambda2, int *innerIter, double *thresh, double *ldot, double *nullBeta, double *gamma, double *eta, int* betaIsZero, int* groupChange, int* isActive, int* useGroup, double *prob, double *betaZero, double *step)
{
    int i, j, k;
    double *theta = malloc(sizeof(double)*ncol[0]);
    double *thetaNew = malloc(sizeof(double)*ncol[0]);
    int startInd = 0;
    double zeroCheck = 0;
    double check = 0;
    int count = 0;
    double t = step[0];
    double diff = 1;
    double norm = 0;
    double uOp = 0;
    double Lnew = 0;
    double Lold = 0;
    double sqNormG = 0;
    double iProd = 0;
    double *etaNew = NULL;
    etaNew = malloc(sizeof(double)*nrow[0]);
    double *etaNull = NULL;
    etaNull = malloc(sizeof(double)*nrow[0]);
    
    for(i = 0; i < numGroup[0]; i++)
    {
        if(useGroup[i] == 1)
        {
            startInd = rangeGroupInd[i];
            

            for(k = 0; k < nrow[0]; k++)
            {
                etaNull[k] = eta[k];
                for(j = startInd; j < rangeGroupInd[i] + groupLen[i]; j++)
                {
                    etaNull[k] = etaNull[k] - X[k + nrow[0] * j] * beta[j];
                }
            }
            
           
            pCalc(nrow, etaNull, prob);
            logitGradCalc(nrow, prob, y, ldot);
            
            double *grad = NULL;
            grad = malloc(sizeof(double)*groupLen[i]);
            
            for(j = 0; j < groupLen[i]; j++)
            {
                grad[j] = 0;
                for(k = 0; k < nrow[0]; k++)
                {
                    grad[j] = grad[j] + X[k + nrow[0] * (j + rangeGroupInd[i])] * ldot[k];
                }
                if(grad[j] < lambda1[0] && grad[j] > -lambda1[0])
                {
                    grad[j] = 0;
                }
                if(grad[j] > lambda1[0])
                {
                    grad[j] = grad[j] - lambda1[0];
                }
                if(grad[j] < -lambda1[0])
                {
                    grad[j] = grad[j] + lambda1[0];
                }
                if(pow(grad[j],2) == pow(lambda1[0],2))
                {
                    grad[j] = 0;
                }
                
            }
            
            zeroCheck = 0;
            for(j = 0; j < groupLen[i]; j++)
            {
                zeroCheck = zeroCheck + pow(grad[j],2);
            }
            
            if(zeroCheck <= pow(lambda2[0],2) * groupLen[i])   
            {
                if(betaIsZero[i] == 0)
                {
                    for(k = 0; k < nrow[0]; k++)
                    {
                        for(j = rangeGroupInd[i]; j < rangeGroupInd[i] + groupLen[i]; j++)
                        {
                            eta[k] = eta[k] - X[k + nrow[0] * j] * beta[j];
                        }
                    }
                }
                betaIsZero[i] = 1;
                for(j = 0; j < groupLen[i]; j++)
                {
                    beta[j + rangeGroupInd[i]] = 0;
                }
            }
            else
            {
                if(isActive[i] == 0)
                {
                    *groupChange = 1;
                }
                isActive[i] = 1;
                
                for(k = 0; k < ncol[0]; k++)
                {
                    theta[k] = beta[k];
                }
                
                betaIsZero[i] = 0;
                double *z = NULL;
                z = malloc(sizeof(double)*groupLen[i]);
                double *U = NULL;
                U = malloc(sizeof(double)*groupLen[i]);
                double *G = NULL;
                G = malloc(sizeof(double)*groupLen[i]);
                double *betaNew = NULL;
                betaNew = malloc(sizeof(double)*ncol[0]);
                
                count = 0;
                check = 1000000;
                
                
                
                while(count <= innerIter[0] && check > thresh[0])
                {
                    
                    count++;
                    
                    pCalc(nrow, eta, prob);
                    logitGradCalc(nrow, prob, y ,ldot);
                    
                    for(j = 0; j < groupLen[i]; j++)
                    {
                        grad[j] = 0;
                        for(k = 0; k < nrow[0]; k++)
                        {
                            grad[j] = grad[j] + X[k + nrow[0] * (j + rangeGroupInd[i])] * ldot[k];
                        }
                        
                    }
                    
                    diff = -1;
                
                    pCalc(nrow, eta, prob);
                    Lold = logitNegLogLikelihoodCalc(nrow, prob, y);
                    
                
                    
                    while(diff < 0)
                    {
                        for(j = 0; j < groupLen[i]; j++)
                        {
                            z[j] = beta[j + rangeGroupInd[i]] - t * grad[j];
                            if(z[j] < lambda1[0] * t && z[j] > -lambda1[0] * t)
                            {
                                z[j] = 0;
                            }
                            if(z[j] > lambda1[0] * t)
                            {
                                z[j] = z[j] - lambda1[0] * t;
                            }
                            if(z[j] < -lambda1[0] * t)
                            {
                                z[j] = z[j] + lambda1[0] * t;
                            }
                        }
                        
                        norm = 0;
                        for(j = 0; j < groupLen[i]; j++)
                        {
                            norm = norm + pow(z[j],2);
                        }
                        norm = sqrt(norm);
                        
                        if(norm != 0){
                            uOp = (1 - lambda2[0]*sqrt((double)groupLen[i])*t/norm);   
                        }
                        else{uOp = 0;}
                        
                        if(uOp < 0)
                        {
                            uOp = 0;
                        }
                        
                        for(j = 0; j < groupLen[i]; j++)
                        {
                            U[j] = uOp*z[j];
                            G[j] = 1/t *(beta[j + rangeGroupInd[i]] - U[j]);
                            
                        }
                        
                  
                        
                        for(j = 0; j < rangeGroupInd[i]; j++)
                        {
                            thetaNew[j] = beta[j];
                        }
                        for(j = rangeGroupInd[i]; j < rangeGroupInd[i] + groupLen[i]; j++)
                        {
                            thetaNew[j] = beta[j] - t * G[j];
                        }
                        
                        for(j = rangeGroupInd[i] + groupLen[i]; j < ncol[0]; j++)
                        {
                            thetaNew[j] = beta[j];
                        }
                        for(k = 0; k < nrow[0]; k++)
                        {
                            etaNew[k] = eta[k];
                            for(j = 0; j < groupLen[i]; j++)
                            {
                                etaNew[k] = etaNew[k] - t*G[j] * X[k + nrow[0]*(rangeGroupInd[i] + j)];
                            }
                        }
                        
                        pCalc(nrow, etaNew, prob);
                        Lnew = logitNegLogLikelihoodCalc(nrow, prob, y);
                        
                        sqNormG = 0;
                        iProd = 0;
                        
                        for(j = 0; j < groupLen[i]; j++)
                        {
                            sqNormG = sqNormG + pow(G[j],2);
                            iProd = iProd + grad[j] * G[j];
                        }
                        
                        diff = Lold - Lnew - t * iProd + t/2 * sqNormG;
                        
                        t = t * gamma[0];
                    }
                    t = t / gamma[0];
                    
                    check = 0;
                    
                    for(j = 0; j < groupLen[i]; j++)
                    {
                        check = check + fabs(theta[j + rangeGroupInd[i]] - U[j]);
                        for(k = 0; k < nrow[0]; k++)
                        {
                            eta[k] = eta[k] - X[k + nrow[0] * (j + rangeGroupInd[i])]*beta[j + rangeGroupInd[i]];
                        }
                        beta[j + rangeGroupInd[i]] = U[j] + count/(count+3) * (U[j] - theta[j + rangeGroupInd[i]]);
                        theta[j + rangeGroupInd[i]] = U[j];
                        
                        for(k = 0; k < nrow[0]; k++)
                        {
                            eta[k] = eta[k] + X[k + nrow[0] * (j + rangeGroupInd[i])]*beta[j + rangeGroupInd[i]];
                        }
                    }
                }
                free(z);
                free(U);
                free(G);
                free(betaNew);
            }
            free(grad);
        }
    }
    betaZeroSolve(nrow, betaZero, eta, prob, thresh, innerIter, y);
    
    free(etaNew);
    free(etaNull);
    free(theta);
    free(thetaNew);
}



int logitNest(double *X, int* y, int* index, int *nrow, int *ncol, int *numGroup, int *rangeGroupInd, int *groupLen, double *lambda1, double *lambda2, double *beta, int *innerIter, int *outerIter, double *thresh, double *outerThresh, double *eta, double *gamma, int *betaIsZero, double* betaZero, double *step)
{
    int i, j, k;
    double oldBetaZero = betaZero[0];
    double* prob = NULL;
    prob = malloc(sizeof(double)*nrow[0]);
    double* nullBeta = NULL;
    nullBeta = malloc(sizeof(double)*ncol[0]);
    int n = nrow[0];
    int p = ncol[0];
    double *ldot = NULL;
    ldot = malloc(sizeof(double)*n);
    int groupChange = 1;
    int* isActive = NULL;
    isActive = malloc(sizeof(int)*numGroup[0]);
    int* useGroup = NULL;
    useGroup = malloc(sizeof(int)*numGroup[0]);
    int* tempIsActive = NULL;
    tempIsActive = malloc(sizeof(int)*numGroup[0]);
    
    for(i = 0; i < numGroup[0]; i++)
    {
        isActive[i] = 0;
        useGroup[i] = 1;
    }
    
   
    int outermostCounter = 0;
    double outermostCheck = 1000000;
    double* outerOldBeta = NULL;
    outerOldBeta = malloc(sizeof(double)*p);
    
    while(groupChange == 1)
    {
        groupChange = 0;
        
        logitSolver(X, y, index, nrow, ncol, numGroup, beta, rangeGroupInd, groupLen, lambda1, lambda2, innerIter, thresh, ldot, nullBeta, gamma, eta, betaIsZero, &groupChange, isActive, useGroup, prob, betaZero, step);
        
        while(outermostCounter < outerIter[0] && outermostCheck > outerThresh[0])
        {
            outermostCounter ++;
            for(i = 0; i < p; i++)
            {
                outerOldBeta[i] = beta[i];
            }
            oldBetaZero = betaZero[0];
            
            for(i = 0; i < numGroup[0]; i++)
            {
                tempIsActive[i] = isActive[i];
            }
            
            
            logitSolver(X, y, index, nrow, ncol, numGroup, beta, rangeGroupInd, groupLen, lambda1, lambda2, innerIter, thresh, ldot, nullBeta, gamma, eta, betaIsZero, &groupChange, tempIsActive, isActive, prob, betaZero, step);
            
            outermostCheck = 0;
            for(i = 0; i < p; i++)
            {
                outermostCheck = outermostCheck + fabs(outerOldBeta[i] - beta[i]);
            }
            outermostCheck = outermostCheck + fabs(oldBetaZero - betaZero[0]);
        }
    }
    
    free(nullBeta);
    free(outerOldBeta);
    free(ldot);
    free(isActive);
    free(useGroup);
    free(tempIsActive);
    free(prob);
    
    return 1;
}



#define P_X                 0
#define P_Y                 1
#define P_INDEX             2
#define P_RANGEGROUPIND     3
#define P_GROUPLEN          4
#define P_LAMBDA1           5
#define P_LAMBDA2           6
#define P_INNERITER         7
#define P_OUTERITER         8
#define P_THRESH            9
#define P_OUTERTHRESH       10
#define P_ETA               11
#define P_GAMMA             12
#define P_STEP              13
#define P_RESET             14
#define R_BETA              0
#define R_BETAISZERO        1

EXTERN_C void mexFunction(int nls, mxArray **plhs, int nrhs, const mxArray **prhs)
{
	int nrow, ncol, numGroup;
	double *X, *y, *lambda1, *lambda2, *thresh, *outerThresh, *eta, *gamma, *step, *beta;
    int *index, *rangeGroupInd, *groupLen, *innerIter, *outerIter, *reset, *betaIsZero;
	nrow = (int)mxGetM(prhs[P_X]);
	ncol = (int)mxGetN(prhs[P_X]);
	numGroup = (int)mxGetM(prhs[P_GROUPLEN]);
        
	plhs[R_BETA] = mxCreateDoubleMatrix(1, ncol, mxREAL);
	plhs[R_BETAISZERO] = mxCreateNumericMatrix(1, ncol, mxINT32_CLASS, 0);
	beta = mxGetPr(plhs[R_BETA]);
	betaIsZero = (int*)mxGetData(plhs[R_BETAISZERO]);
	X = mxGetPr(prhs[P_X]);
	y = mxGetPr(prhs[P_Y]);
    index = (int*)mxGetData(prhs[P_INDEX]);
    rangeGroupInd = (int*)mxGetData(prhs[P_RANGEGROUPIND]);
    groupLen = (int*)mxGetData(prhs[P_GROUPLEN]);
    lambda1 = mxGetPr(prhs[P_LAMBDA1]);
    lambda2 = mxGetPr(prhs[P_LAMBDA2]);
    innerIter = (int*)mxGetData(prhs[P_INNERITER]);
    outerIter = (int*)mxGetData(prhs[P_OUTERITER]);
    thresh = mxGetPr(prhs[P_THRESH]);
    outerThresh = mxGetPr(prhs[P_OUTERTHRESH]);
    eta = mxGetPr(prhs[P_ETA]);
    gamma = mxGetPr(prhs[P_GAMMA]);
    step = mxGetPr(prhs[P_STEP]);
    reset = (int*)mxGetData(prhs[P_RESET]);
	linNest(X, y, index, &nrow, &ncol, &numGroup, rangeGroupInd, 
            groupLen, lambda1, lambda2, beta, innerIter, outerIter, thresh, 
            outerThresh, eta, gamma, betaIsZero, step, reset);
}


