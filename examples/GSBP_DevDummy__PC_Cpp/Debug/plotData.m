%% plot the dummy measurement data from the GSBP devel/example device
close all;

data = csvread('Dummy_Data.csv');
nSamples    = data(:,1);
dummyData   = data(:,2);

figure();
plot(nSamples);
grid on;
legend('Number of Values');

figure();
plot(dummyData);
grid on;
legend('dummyData');

