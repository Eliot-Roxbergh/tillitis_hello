# Use TKey for PAM authentication (with tkey ssh-agent)

The [tkey-ssh-agent](https://github.com/tillitis/tillitis-key1-apps/tree/main/cmd/tkey-ssh-agent) is an application (which runs simultaneously on the TKey and the computer) that enables us to use the TKey for key-based authentication, instead of directly interacting with a private key on the local computer.
That is, the TKey is taking on a role similar to that of an HSM or TPM, although technically, the private key(s) is re-derived on each boot-up as it has no lasting user state.¹

With an ssh-agent, we can however do more than just SSH, for instance, there is a PAM module ([pam_ssh_agent_auth](https://manpages.ubuntu.com/manpages/bionic/man8/pam_ssh_agent_auth.8.html))
that allows the use of an ssh-agent for authentication.
Let's see if we can use this to authenticate to our Linux device using the TKey.
We could then use the TKey in conjunction with a password (2FA), or for convenience, it could provide an alternative to passwords for some applications.

Therefore, in this document we will: \
First, setup the TKey for SSH connections (where signing is done on the TKey with its key). \
Second, by the same mechanism, we will use the TKey for authentication to PAM: replacing the need to enter a password for the `sudo` command.

¹ The keys are generated based on the internal Unique Device Secret, a User Supplied Secret, and the application hash; and by using the application hash in key generation, it is ensured that the application uploaded has not been tampered with since last time.


## The TKey ssh-agent

The tkey-ssh-agent runs in the background, and once called upon via its socket, it transfers the application to the TKey and starts the authentication procedure.
The TKey then generates an ed25519 key-pair and performs cryptographic signing as necessary.

Specifically, the TKey generates a key-pair based on its internal identifier (i.e. Unique Device Secret, UDS), the hash of the TKey app, and optional user input (i.e. User Supplied Secret, USS) [1].
For the tkey-ssh-agent the USS is optional, but if used, it functions as a passphrase; otherwise the private keys won't match.

[1] - <https://tillitis.se/2023/03/31/on-tkey-key-generation/>

### i) SSH: use TKey signing via ssh-agent

- Build the tkey-ssh-agent, and download Golang >= 1.19 if not already installed

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

- Add the TKey internal public key to the servers you'd like to access over SSH (here your local machine)

```bash
# add pubkey to ~/.ssh/authorized_keys
#   example line: ssh-ed25519 AAAA5Ns36SKds24ovMDXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX4Asdv/U My-Tillitis-TKey
./tkey-ssh-agent  --show-pubkey 1>>  ~/.ssh/authorized_keys

chmod 700 ~/.ssh
chmod 600 ~/.ssh/authorized_keys
sudo systemctl start ssh
```

- Start the SSH agent. When a request is made to the agent socket, the application will be transferred to the TKey and signing can take place

```bash
./tkey-ssh-agent -a ~/.ssh/agent.sock &
```

- Finally, connect to your local machine using SSH (via this tkey-ssh-agent socket). \
Run the following command and press on the TKey to perform signing and finish the login (as prompted by the tkey-ssh-agent program).

```bash
# (add -vvv for verbose client, and if issues check "journalctl -u ssh.service")
SSH_AUTH_SOCK="$HOME/.ssh/agent.sock" ssh -F /dev/null $USER@localhost
```


### ii) sudo: use TKey for authentication

Once the ssh-agent is up and running, we can use it for more than just regular ssh, such as authenticating to PAM.
It is thereby possible to use the TKey for different steps in the Linux authentication, here we will use the TKey as an alternative to passwords for running the 'sudo' command.

- Enable the tkey-ssh-agent to automatically start for the current user, creating a socket at login

```bash
cd tillitis-key1-apps
make all
sudo make install
cd ..

systemctl --user start tkey-ssh-agent.service
systemctl --user enable tkey-ssh-agent.service
#socket should now be here: `/run/user/$UID/tkey-ssh-agent/sock`
```

- We set the authorized keys path to `/etc/ssh/sudo_authorized_keys`, keeping it separate from ssh's trusted keys. Note, any keys listed in this file will be used to gain sudo privileges in this example.

```bash
#(if not already copied, you may also use `ssh-add -L` to get the public key from the ssh-agent)
sudo cp ~/.ssh/authorized_keys /etc/ssh/sudo_authorized_keys
```

- Install the PAM module `ssh-agent-auth`

```bash
sudo apt install libpam-ssh-agent-auth
```

- Add the alternative authentication method (ssh-agent-auth) for sudo by adding the PAM module as a step in its PAM config (i.e. /etc/pam.d/sudo).
The tkey can then be used in conjunction to, or instead of a password, by adding a new line before the password authentication step: `pam_unix.so` (common-auth). \
To use the tkey in conjunction with a password we set the module to be "required", for instance \
`auth required pam_ssh_agent_auth.so file=/etc/ssh/sudo_authorized_keys`.
On the other hand, as shown in the snippet below, we can instead use the more complicated PAM syntax (e.g. `[success=2 default=ignore]`),
to on success skip the two next modules ignoring the password requirement, and on failure ignore the result of this module and continue with password authentication instead. \
It should be possible to use this authentication for any service using PAM, not only sudo - see other configs in `/etc/pam.d/` - as long as `$SSH_AUTH_SOCK` is set.
The new configuration is applied as soon as the file is saved, and in this case, the new authentication step will be checked once 'sudo' is run.

```bash
#/etc/pam.d/sudo

# ...

#add this line before pam_unix.so and change success=2 to the number of modules to skip on success
#note: "$SSH_AUTH_SOCK" must always point to the socket file
auth [success=2 default=ignore] pam_ssh_agent_auth.so file=/etc/ssh/sudo_authorized_keys debug

# (in this example we want to skip these 2 modules on success, replacing password authentication with TKey)
# check password
auth	[success=1 default=ignore]	pam_unix.so nullok
# default deny
auth	requisite			pam_deny.so

# -> (success: either tkey or password authentication succeeded)

# ...

```

- Contrary to Ubuntu manpage, you need to add the following to /etc/sudoers, by running `sudo visudo /etc/sudoers`. This is necessary as to properly inherit SSH_AUTH_SOCK. (The Ubuntu 22.04 system uses sudo v1.9.9 and still requires this step)

```bash
Defaults        env_keep += "SSH_AUTH_SOCK"
```

- We are now ready to try to authenticate using the TKey

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

**If issues arise, check log: e.g. `sudo journalctl -f` or `/var/log/auth.log` and the status of tkey with `systemctl status tkey-ssh-agent`.**
The most common error is _"pam_ssh_agent_auth: No ssh-agent could be contacted"_, usually regarding the environment variable SSH_AUTH_SOCK not set or inherited properly.
Make sure that it's set and that the socket indeed exists, for the sudo command as shown above the configuration option 'env_keep' is also required.

¹ The environment variable SSH_AUTH_SOCK needs to be set, which can be done in several ways depending on your system or who calls it. In addition to regular export in e.g. _~/.profile_, this can also be done in a PAM configuration file [1] such as in _/etc/security/pam_env.conf_ or _/etc/environment_, or by loading your own with _pam_env_ [2]. It is also possible to use systemd's _environment.d_ [3]. For an overview of environment variables [see for instance [4]](https://wiki.archlinux.org/title/Environment_variables).

[1] - https://linux.die.net/man/8/pam_env \
note: pam has a special syntax for env config, for example add the following line to the environment .conf file (e.g. /etc/security/pam_env.conf): \
`SSH_AUTH_SOCK DEFAULT=/home/user/.ssh/agent.sock`\
[2] - Optionally, you can create your own config file instead and point to it from /etc/pam.d/sudo: \
`session required pam_env.so readenv=1 envfile=my-config-file.conf user_readenv=0` \
[3] - https://www.freedesktop.org/software/systemd/man/environment.d.html \
[4] - https://wiki.archlinux.org/title/Environment_variables
