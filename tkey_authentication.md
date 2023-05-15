# Use TKey for PAM authentication (with tkey ssh-agent)

There's an ssh-agent app available in the tillitis-key1-apps repo.
This enables us to use the TKey for key-based authentication over ssh.
Moreover, by using the PAM module ssh-agent-auth it's possible to login your Linux system using the TKey and this mechanism.
For instance, replacing passwords or as a 2FA token in conjunction with current authentication.


## 1. Use ssh TKey signing via ssh-agent


- build tkey-ssh-agent, and download Golang >= 1.19 if not already installed

```
cd tillitis-key1-apps
# ----> DL -> https://go.dev/dl/
tar xvf go*.tar.gz
export PATH="$PWD/go/bin/:$PATH"
make all
```


- Get ssh

```
sudo apt install -y ssh
sudo systemctl disable ssh
```

- Add the TKey internal pubkey to the servers you'd like to access over SSH (here your local machine)

```
./tkey-ssh-agent  --show-pubkey
# ----> add to ~/.ssh/authorized_keys
#   example: ssh-ed25519 AAAA5Ns36SKds24ovMDXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX4Asdv/U My-Tillitis-TKey
chmod 700 ~/.ssh
chmod 600 ~/.ssh/authorized_keys
sudo systemctl start ssh
```

- Create the ssh-agent socket for signing via TKey

```
./tkey-ssh-agent -a ~/.ssh/agent.sock &
```

- Finally, connect to your local machine using SSH (via this tkey ssh-agent socket). \
Run this command and press on TKey to perform sign and finish the login (as prompted by the tkey-ssh-agent program).

```
# (add -vvv for verbose client, and if issues check "sudo journalctl -u ssh.service")
SSH_AUTH_SOCK=/home/$USER/.ssh/agent.sock ssh -F /dev/null $USER@localhost
```


## 2. Add ssh-agent to PAM for login

Once the ssh-agent is working, we can use it for other things that just regular ssh.
Such as use it to authenticate to PAM.

- Make sure ssh-auth socket is up (started in last section) and that the public key is in the authorized_keys file \
In this example, put the public key in /etc/ssh/sudo_authorized_keys.

- Install PAM module ssh-agent-ath

```
sudo apt install libpam-ssh-agent-auth
```

- Add the alternative authentication method (ssh-agent-auth) for sudo by adding the PAM module as a step in its PAM config (i.e. /etc/pam.d/sudo). \
As far as I know, the new configuration is applied as soon as the file is saved, try with 'sudo'. \
By setting the module as 'sufficient' the user can either use the TKey or password (or whatever they used before). \
On the other hand, if we were to set the module as 'required' the TKey would be used in addition to password, i.e. 2FA (NOTE: careful not to lock yourself out of the system). \
Similarily, we can use this authentication for any service using PAM, not only sudo; see other configs in /etc/pam.d/.

```
#/etc/pam.d/sudo
#note: uses ssh-agent socket at "$SSH_AUTH_SOCK"
auth sufficient pam_ssh_agent_auth.so file=/etc/ssh/sudo_authorized_keys
```

- Contrary to Ubuntu manpage (TODO! why? ??), you need to add the following to /etc/sudoers (The Ubuntu 22.04 system uses sudo 1.9.9, but still requires this step)

```
#/etc/sudoers
Defaults        env_keep += "SSH_AUTH_SOCK"
```

- Now make sure to set $SSH_AUTH_SOCK either globaly (e.g. .bashrc export) or for each call to sudo. This variable must point to the active ssh-agent socket, created by tkey-ssh-agent. \
Try to authenticate using TKey:

```
SSH_AUTH_SOCK=~/.ssh/agent.sock sudo echo
```


**If issues arise, check log: /var/log/auth.log and add 'debug' to the line in /etc/pam.d/sudo**

## 3. Make permanent

This was just a proof-of-concept, you might also need to do the following:

- Create a ssh-agent socket at system start: Run `make install` in the tkey-apps repo, which creates a new systemd service (see their tkey-ssh-agent.service.tmpl)

- PAM config: Use pam-ssh-agent-auth for the service you'd like, such as /etc/pam.d/login for system login. \
I'd assume that you'd either (e.g.) put the module line above the "@include common-password" line in /etc/pam.d/login, \
or use pam-auth-update(8) to apply globally to all PAM supported services. \
You may also want to limit this authentication method to certain services or certain users.

The most common error is _"pam_ssh_agent_auth: No ssh-agent could be contacted"_, usually regarding the
SSH_AUTH_SOCK is not set or inherited properly.


SSH_AUTH_SOCK needs to be set, however this may depend on your system. For instance this can be done in PAM with /etc/security/pam_env.conf, or /etc/environment, or by loading a file pam_env [1] ([2]). It can also be done with systemd's environment.d [3]. Or simply add to /etc/profile.
For an overview see [4].
(TODO! not tested).

[1] - https://linux.die.net/man/8/pam_env \
note: pam has a special syntax for env config, example add the following to .conf file: \
`SSH_AUTH_SOCK DEFAULT=/home/user/.ssh/agent.sock`\
[2] - Optionally, create your own config file and add to /etc/pam.d/sudo: \
`session required pam_env.so readenv=1 envfile=X.conf user_readenv=0` \
[3] - https://www.freedesktop.org/software/systemd/man/environment.d.html \
[4] - https://wiki.archlinux.org/title/Environment_variables
