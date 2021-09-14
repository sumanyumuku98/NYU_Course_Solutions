import tqdm
import hydra
import os

import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim

import torchvision.transforms as T

import numpy as np
import gym
import pybulletgym  # register PyBullet enviroments with open ai gym

from reacher_env import ReacherDaggerEnv
from utils import weight_init, ExpertBuffer


env=ReacherDaggerEnv()
states=[]
for _ in range(20):
    states.append(env.reset())

for i in range(len(states)-1):
    print((states[i]==states[i+1]).all())

# env=gym.make("ReacherPyBulletEnv-v0")
# print(env.metadata)
# env.render()
# env.reset()
# for _ in range(10000):
#     env.render()
#     env.step(env.action_space.sample())
# env.close()

