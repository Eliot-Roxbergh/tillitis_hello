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

- Install SSH

```
sudo apt install -y ssh
sudo systemctl disable ssh
```

- Add the TKey internal pubkey to the servers you'd like to access over SSH (here your local machine)

```
./tkey-ssh-agent  --show-pubkey
# --> add to ~/.ssh/authorized_keys
#       example:
#        echo "ssh-ed25519 AAAA5Ns36SKds24ovMDXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX4Asdv/U My-Tillitis-TKey" >>  ~/.ssh/authorized_keys
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


## 2. Use for authentication

Once the ssh-agent is working, we can use it for other things than just regular ssh.
Such as use it to authenticate to PAM.

- Run TKey ssh agent and enable on boot, which creates a socket at login.

```
cd tillitis-key1-apps
sudo make install
cd ..

systemctl --user start tkey-ssh-agent.service
systemctl --user enable tkey-ssh-agent.service
#socket should now be here: `/run/user/$UID/tkey-ssh-agent/sock`
```

- In this example, put the public key in /etc/ssh/sudo_authorized_keys.

```
sudo cp ~/.ssh/authorized_keys /etc/ssh/sudo_authorized_keys
```

- Install PAM module ssh-agent-ath

```
sudo apt install libpam-ssh-agent-auth
```

- Add the alternative authentication method (ssh-agent-auth) for sudo by adding the PAM module as a step in its PAM config (i.e. /etc/pam.d/sudo)[1]. \
The new configuration is applied as soon as the file is saved, try with 'sudo'. \
By setting the module as 'sufficient' the user can either use the TKey (and remaining steps are ignored) or password (or whatever they used before). \
On the other hand, if we were to set the module as 'required' the TKey would be used in addition to password, i.e. 2FA (NOTE: careful not to lock yourself out of the system). \
Similarily, we can use this authentication for any service using PAM, not only sudo; see other configs in /etc/pam.d/.

```
#/etc/pam.d/sudo
#note: uses ssh-agent socket at env var "$SSH_AUTH_SOCK"
auth sufficient pam_ssh_agent_auth.so file=/etc/ssh/sudo_authorized_keys debug
```

- Contrary to Ubuntu manpage (TODO! why? ??), you need to add the following to /etc/sudoers (The Ubuntu 22.04 system uses sudo 1.9.9, but still requires this step)

```
#sudo visudo /etc/sudoers
Defaults        env_keep += "SSH_AUTH_SOCK"
```

- Now make sure to set $SSH_AUTH_SOCK either globaly (e.g. .bashrc export) or for each call to sudo. This variable must point to the active ssh-agent socket, created by tkey-ssh-agent. \
Try to authenticate using TKey:

```
SSH_AUTH_SOCK=/run/user/$UID/tkey-ssh-agent/sock sudo echo
```

- Set SSH_AUTH_SOCK, e.g. in /etc/profile or ~/.profile [2]
```
echo "export SSH_AUTH_SOCK=/run/user/$UID/tkey-ssh-agent/sock" >> ~/.profile
```

PAM config: it should be possible to use pam-ssh-agent-auth for any service you'd like, such as /etc/pam.d/sudo or /etc/pam.d/login. \
I can't say if it's a good/bad idea however.

**If issues arise, check log: e.g. `sudo journalctl -f` or `/var/log/auth.log` and the status of tkey with `systemctl status tkey-ssh-agent`** \
The most common error is _"pam_ssh_agent_auth: No ssh-agent could be contacted"_, usually regarding the SSH_AUTH_SOCK not set or inherited properly.

[1] - I think that you'd should either put the module line above the "@include common-auth" line in /etc/pam.d/login, \
or create a profile for pam-auth-update(8) to apply globally to all PAM supported services. \
You may also want to limit this authentication method to certain services or certain users.
[2] - SSH_AUTH_SOCK needs to be set, which can be done in multiple ways depending on your system or who calls it. For instance, this can be done in PAM in /etc/security/pam_env.conf or /etc/environment, or by loading a file with pam_env [2a], ([2b]). It can also be done with systemd's environment.d [2c]. Or simply add to /etc/profile. \
For an overview see [2d]. \
[2a] - https://linux.die.net/man/8/pam_env \
note: pam has a special syntax for env config, example add the following to .conf file: \
`SSH_AUTH_SOCK DEFAULT=/home/user/.ssh/agent.sock`\
[2b] - Optionally, create your own config file and add to /etc/pam.d/sudo: \
`session required pam_env.so readenv=1 envfile=X.conf user_readenv=0` \
[2c] - https://www.freedesktop.org/software/systemd/man/environment.d.html \
[2d] - https://wiki.archlinux.org/title/Environment_variables
