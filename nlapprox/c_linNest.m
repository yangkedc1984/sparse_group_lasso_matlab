function [beta, betaIsZero] = c_linNest(X, y, index, nrow, ncol, numGroup, rangeGroupInd, ...
    groupLen, lambda1, lambda2, innerIter, outerIter, thresh, ...
    outerThresh, eta, gamma, step, reset )
    beta = zeros(1, ncol);
    betaIsZero = zeros(1, ncol);
    
    if coder.target('MATLAB')
        fprintf('Running Matlab Code...\n\n');
    else
        coder.updateBuildInfo('addSourceFiles', 'code.c');
        coder.updateBuildInfo('addIncludePaths', 'C:\Data\mex_function');
        fprintf('Running Custom C Code...\n\n');
        coder.ceval('linNest', coder.ref(X), ...
            coder.ref(y), ...
            coder.ref(index), ...
            coder.ref(nrow), ...
            coder.ref(ncol), ...
            coder.ref(numGroup), ...
            coder.ref(rangeGroupInd), ...
            coder.ref(groupLen), ...
            coder.ref(lambda1), ...
            coder.ref(lambda2), ...
            coder.ref(beta), ...
            coder.ref(innerIter), ...
            coder.ref(outerIter), ...
            coder.ref(thresh), ...
            coder.ref(outerThresh), ...
            coder.ref(eta), ...
            coder.ref(gamma), ...
            coder.ref(betaIsZero), ...
            coder.ref(step), ...
            coder.ref(reset));
    end
end