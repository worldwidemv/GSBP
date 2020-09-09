// plot the dummy measurement data from the GSBP devel/example device
fig = scf(0); close(fig);
fig=gcf();while(fig.figure_id ~=0), close(fig); fig=gcf(); end, close(fig);

data = fscanfMat("Dummy_Data.csv");
nSamples    = data(:,1);
dummyData   = data(:,2);

scf();
plot(nSamples);
xgrid(4);
legend('Number of Values');

scf();
plot(dummyData);
xgrid(4);
legend('dummyData');
