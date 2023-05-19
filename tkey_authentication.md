# Use TKey for PAM authentication (with tkey ssh-agent)

There is an ssh-agent app available in the [tillitis-key1-apps](https://github.com/tillitis/tillitis-key1-apps) repo.
This enables us to use the TKey for key-based authentication over ssh.
Moreover, by using the PAM module "[pam_ssh_agent_auth](https://manpages.ubuntu.com/manpages/bionic/man8/pam_ssh_agent_auth.8.html)" it is possible to login to the Linux system using the TKey ssh-agent.
For instance, replacing passwords or as a 2FA token in conjunction with a password (or other methods).

In this document we will: \
First, setup the TKey for SSH connections (where signing is done on the TKey with its key). \
Second, by the same mechanism, we will use the TKey for authentication to PAM: replacing the need to enter a password for the `sudo` command.

## The TKey ssh-agent 

The tkey-ssh-agent runs in the background and once called upon via its socket, it transfers the application to the TKey and starts the authentication.
The TKey then generates a ed25519 key-pair and performs cryptographic signatures.

The TKey generates a key-pair based on its internal identifier (i.e. Unique Device Secret, UDS), the hash of the TKey app, and optional user input (i.e. User Supplied Secret, USS) [1].
For the tkey-ssh-agent the USS is optional, but if used, it functions as a passphrase; otherwise the private keys won't match.

[1] - <https://tillitis.se/2023/03/31/on-tkey-key-generation/>

## 1. SSH: use TKey signing via ssh-agent

- build tkey-ssh-agent, and download Golang >= 1.19 if not already installed

```bash
cd tillitis-key1-apps
# ---> DL golang: https://go.dev/dl/
#   tar xvf go*.tar.gz
#   export PATH="$PWD/go/bin/:$PATH"
make all
```

- Install SSH

```bash
sudo apt install -y ssh
sudo systemctl disable ssh
```

- Add the TKey internal pubkey to the servers you'd like to access over SSH (here your local machine)

```bash
# add to ~/.ssh/authorized_keys
#   example:
#   echo "ssh-ed25519 AAAA5Ns36SKds24ovMDXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX4Asdv/U My-Tillitis-TKey" >>  ~/.ssh/authorized_keys
# 
# It's also possible to add the --uss option to require a passphrase for the key pair
./tkey-ssh-agent  --show-pubkey 1>>  ~/.ssh/authorized_keys
chmod 700 ~/.ssh
chmod 600 ~/.ssh/authorized_keys
sudo systemctl start ssh
```

- Create the ssh-agent socket for signing via TKey

```bash
./tkey-ssh-agent -a ~/.ssh/agent.sock &
```

- Finally, connect to your local machine using SSH (via this tkey ssh-agent socket). \
Run this command and press on TKey to perform sign and finish the login (as prompted by the tkey-ssh-agent program).

```bash
# (add -vvv for verbose client, and if issues check "journalctl -u ssh.service")
SSH_AUTH_SOCK=/home/$USER/.ssh/agent.sock ssh -F /dev/null $USER@localhost
```

## 2. sudo: use TKey for authentication

Once the ssh-agent is working, we can use it for other things than just regular ssh.
Such as use it to authenticate to PAM.

- Run TKey ssh agent and enable on boot, which creates a socket at login

```bash
cd tillitis-key1-apps
sudo make install
cd ..

systemctl --user start tkey-ssh-agent.service
systemctl --user enable tkey-ssh-agent.service
#socket should now be here: `/run/user/$UID/tkey-ssh-agent/sock`
```

- In this example, we use the authorized keys path `/etc/ssh/sudo_authorized_keys`. Note, any keys listed in this file can be used to gain sudo privileges.

```bash
#note: to get the public key from the ssh-agent, type `ssh-add -L`
sudo cp ~/.ssh/authorized_keys /etc/ssh/sudo_authorized_keys
```

- Install PAM module ssh-agent-auth

```bash
sudo apt install libpam-ssh-agent-auth
```

- Add the alternative authentication method (ssh-agent-auth) for sudo by adding the PAM module as a step in its PAM config (i.e. /etc/pam.d/sudo).
The tkey can then be used in conjunction to, or instead of a password, by adding a new line before the password authentication step: `pam_unix.so` (common-auth). \
To use the tkey in conjunction with a password we set the module to be "required", for instance \
`auth required pam_ssh_agent_auth.so file=/etc/ssh/sudo_authorized_keys`.
On the other hand, as shown in the snippet below, we can instead use the more complicated PAM syntax (e.g. `[success=2 default=ignore]`),
to on success skip the two next modules ignoring the password requirement, and on failure ignore the result of this module and continue with password authentication instead. \
We can use this authentication for any service using PAM, not only sudo; see other configs in `/etc/pam.d/` - as long as `$SSH_AUTH_SOCK` is set.
The new configuration is applied as soon as the file is saved, try with 'sudo'.

```bash
#/etc/pam.d/sudo
# ...

#add this line before pam_unix.so and change success=2 to the number of modules to skip on success
#note: "$SSH_AUTH_SOCK" must always point to the socket file
auth [success=2 default=ignore] pam_ssh_agent_auth.so file=/etc/ssh/sudo_authorized_keys debug

# check password
auth	[success=1 default=ignore]	pam_unix.so nullok
# default deny
auth	requisite			pam_deny.so

# -> (success either tkey or password authentication succeeded)
# ...
```

- Contrary to Ubuntu manpage (?), you need to add the following to `/etc/sudoers` (The Ubuntu 22.04 system uses sudo v1.9.9 and still requires this step)

```bash
#sudo visudo /etc/sudoers
Defaults        env_keep += "SSH_AUTH_SOCK"
```

- Try to authenticate using the TKey

```bash
SSH_AUTH_SOCK=/run/user/$UID/tkey-ssh-agent/sock sudo echo SUCCESS!
```

- Finally, let's permanently set `$SSH_AUTH_SOCK`. This variable must point to the active ssh-agent socket, created by the tkey-ssh-agent service. One alternative is to set the variable in /etc/profile or ~/.profile.¹

```bash
echo "export SSH_AUTH_SOCK=/run/user/$UID/tkey-ssh-agent/sock" >> ~/.profile
source ~/.profile
sudo -k #remove cached credentials
sudo echo SUCCESS!
```

PAM config: it should be possible to use pam-ssh-agent-auth for any service you'd like, such as /etc/pam.d/sudo or /etc/pam.d/login.

**If issues arise, check log: e.g. `sudo journalctl -f` or `/var/log/auth.log` and the status of tkey with `systemctl status tkey-ssh-agent`.**
The most common error is _"pam_ssh_agent_auth: No ssh-agent could be contacted"_, usually regarding the SSH_AUTH_SOCK not set or inherited properly.

¹ SSH_AUTH_SOCK needs to be set, which can be done in several ways depending on your system or who calls it. In addition to simple export in e.g. _~/.profile_, this can be done in a PAM configuration file [1] such as in _/etc/security/pam_env.conf_ or _/etc/environment_, or by loading your own with _pam_env_ [2]. It can also be done with systemd's _environment.d_ [3]. For an overview see [4].

[1] - <https://linux.die.net/man/8/pam_env> \
note: pam has a special syntax for env config, example add the following to .conf file: \
`SSH_AUTH_SOCK DEFAULT=/home/user/.ssh/agent.sock`\
[2] - Optionally, create your own config file and add to /etc/pam.d/sudo: \
`session required pam_env.so readenv=1 envfile=X.conf user_readenv=0` \
[3] - <https://www.freedesktop.org/software/systemd/man/environment.d.html> \
[4] - <https://wiki.archlinux.org/title/Environment_variables>
