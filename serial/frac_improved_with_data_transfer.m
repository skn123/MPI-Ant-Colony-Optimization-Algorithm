% Ant Colony Optimization Algorithm with MPI
% Schaer Marc

% Compute fraction of improved serial code without considering data transfer

nants = [1 2 4 8 16];
nodes = [1 2 4 8 16];
c = 1000
result = zeros(size(nodes,2),size(nants,2))
L = 100
l = 50

for i = 1:size(nants,2)
    for j = 1:size(nodes,2)
        result(j,i) = (2*(c+L*nodes(j)))/(2*(c+L*nodes(j)) + (L*l*c*(nants(i))/nodes(j)));
    end
end

disp(result)

figure
plot(nodes,result(:,5))%nodes,speedup32,nodes,speedup64)
legend('16 ants', 'Location', 'northwest')
title('Percentages of improved serial code execution with data transfer')
xlabel('Nb of Nodes')
ylabel('% of serial code')