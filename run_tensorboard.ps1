param(
    [string]$LogDir = "C:\Users\Kaboom\Downloads\CombatLearningAgent-master\CombatLearningAgent-master\Intermediate\LearningAgents\TensorBoard\runs",
    [int]$Port = 6006,
    [string]$UEPython = "C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\ThirdParty\Python3\Win64\python.exe"
)

& "$UEPython" -m tensorboard.main --logdir "$LogDir" --port $Port
