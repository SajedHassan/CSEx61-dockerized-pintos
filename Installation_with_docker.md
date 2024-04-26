# Installation Guidelines

1) Install Docker Desktop from https://www.docker.com/products/docker-desktop/
2) Pull the pinto image `sajedalmorsy/pintos:1.0` from Docker Hub:
```
sudo docker pull sajedalmorsy/pintos:1.0
```
3) Go to the place in where you would like to install the project
Note: You may install it in any place as per your preferences \
ex:
```
cd ~
```
4) Clone the repo:
```
git clone git@github.com:SajedHassan/CSEx61-dockerized-pintos.git
```
5) From the directory, in which the repo is cloned, run a container from the pulled image and attach the repo as a volume:
```
sudo docker run --platform linux/amd64 --rm -it -v "$(pwd)/CSEx61-dockerized-pintos:/root/pintos" a85bf0a348d6
```
Note: If you got an error which is related to specifying container image platform use this command instead:
```
sudo docker run --rm -it -v "$(pwd)/CSEx61-dockerized-pintos:/root/pintos" a85bf0a348d6
```
6) You should now be inside of the running container. Navigate to:
```
cd ~/pintos
```
7) Update the permissions and exclude it from git:
```
git config --global --add safe.directory /root/pintos
chmod -R 777 .
git config core.filemode false
```
8) Build the utils:
```
cd ./src/utils
make clean
make
```
9) Check out the branch of the current project phase.\
For Phase 1 (Threads), checkout `threads-dockerized` \
For Phase 1 (Userprog), checkout `user-prog-dockerized`
```
git checkout threads-dockerized
```
10) Build the current phase code:
```
cd ~/pintos/src/threads/
make clean
make
pintos run alarm-multiple
```
Or, for phase 2
```
cd ~/pintos/src/userprog/
make clean
make
```

Finally, after making the required code changes, run the following to test you code and calculate your grade from the directory that corresponds to the current phase, `~/pintos/src/threads` or `~/pintos/src/userprog`:
```
make grade
```


<br><br><br><br>
Notes:
- If you see permission issue at any stage, run the following from the `~/pintos` directory inside the container:
```
chmod -R 777 .
```
- For phase 2 (Userprog), you may need to do the build steps of both phases as they do depend on each others in some areas.
