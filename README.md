# Introdução a Linux Device Drivers

Para carregar esse módulo ao kernel, você deve executar:

```(shell)
make 
sudo insmod hello.ko
```

Para descarregar esse módulo, insira os seguintes comandos no terminal:

```(shell)
sudo rmmod hello.ko
```

Para debuggar o módulo, pode-se usar os seguintes comandos:

```(shell)
lsmod
modinfo
```