"""
[CSCI-GA 3033-090] Special Topics: Deep Reinforcement Learning

Homework - 1, DAgger
Deadline: Sep 17, 2021 11:59 PM.

Complete the code template provided in dagger.py, with the right 
code in every TODO section, to implement DAgger. Attach the completed 
file in your submission.
"""

import tqdm
import hydra
import os
os.environ["HYDRA_FULL_ERROR"] = "1"

import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim

import torchvision.transforms as T

import numpy as np
# import argparse

from reacher_env import ReacherDaggerEnv
from utils import weight_init, ExpertBuffer


class CNN(nn.Module):
    def __init__(self):
        super(CNN, self).__init__()
        # TODO define your own network
        self.relu=nn.ReLU()
        self.conv1=nn.Conv2d(in_channels=3, out_channels=32, kernel_size=(3,3), padding=1)
        self.conv2=nn.Conv2d(in_channels=32, out_channels=128, kernel_size=(3,3), padding=1)
        self.conv3=nn.Conv2d(in_channels=128, out_channels=32, kernel_size=(3,3), padding=1)
        self.linear=nn.Linear(32*15*20, 2)
        self.maxPool=nn.MaxPool2d(kernel_size=(2,2))
        self.tanh=nn.Tanh()
        self.apply(weight_init)

    def forward(self, x):
        # Normalize
        x = x / 255.0 - 0.5
        # TODO pass it forward through your network.
        x=self.relu(self.conv1(x))
        x=self.maxPool(x)
        x=self.relu(self.conv2(x))
        x=self.maxPool(x)
        x=self.relu(self.conv3(x))
        x=self.linear(x.view(x.shape[0], -1))
        x=self.tanh(x)
        
        return x



def initialize_model_and_optim(cfg):
    # TODO write a function that creates a model and associated optimizer
    # given the config object.
    
    #Initialize Model
    model=CNN()
    
    # Initialize Optimizer
    optimizer=optim.Adam(model.parameters(), lr=cfg.lr, weight_decay=1e-4)
    
    return model, optimizer


class Workspace:
    def __init__(self, cfg):
        self._work_dir = os.getcwd()
        print(f'workspace: {self._work_dir}')

        self.cfg = cfg
        save_dir=self.cfg.save_dir
        if not os.path.exists(save_dir):
            os.makedirs(save_dir)
            # self.cfg.save_path=os.path.join(save_dir, "policy.pt")

        self.device = torch.device(self.cfg.device)
        self.train_env = ReacherDaggerEnv()
        self.eval_env = ReacherDaggerEnv()
        
        ## edit cfg to set action space dim and obs space dim
        # cfg.input_dim=train_env.observation_space.shape
        # cfg.action_space=train_env.action_space.shape

        self.expert_buffer = ExpertBuffer(self.cfg.experience_buffer_len, 
                                          self.train_env.observation_space.shape,
                                          self.train_env.action_space.shape)
        
        self.model, self.optimizer = initialize_model_and_optim(self.cfg)

        # TODO: define a loss function
        self.loss_function = nn.MSELoss()

        self.transforms = T.Compose([
            T.RandomResizedCrop(size=(60, 80), scale=(0.95, 1.0)),
        ])
        self.eval_transforms = T.Compose([
            T.Resize(size=(60, 80))
        ])

    def eval(self):
        # A function that evaluates the 
        # Set the DAgger model to evaluation
        self.model.eval()

        avg_eval_reward = 0.
        avg_episode_length = 0.
        for _ in range(self.cfg.num_eval_episodes):
            eval_reward = 0.
            ep_length = 0.
            obs_np = self.eval_env.reset()
            # Need to be moved to torch from numpy first
            obs = torch.from_numpy(obs_np).float().to(self.device).unsqueeze(0)
            t_obs = self.eval_transforms(obs)
            with torch.no_grad():
                action = self.model(t_obs)
            done = False
            while not done:
                # Need to be moved to numpy from torch
                if action.device=="cuda":
                    action = action.squeeze().detach().cpu().numpy()
                else:
                    action=action.squeeze().detach().numpy()
                    
                obs, reward, done, info = self.eval_env.step(action)
                obs = torch.from_numpy(obs).float().to(self.device).unsqueeze(0)
                t_obs = self.eval_transforms(obs)
                with torch.no_grad():
                    action = self.model(t_obs)
                eval_reward += reward
                ep_length += 1.
            avg_eval_reward += eval_reward
            avg_episode_length += ep_length
        avg_eval_reward /= self.cfg.num_eval_episodes
        avg_episode_length /= self.cfg.num_eval_episodes
        return avg_eval_reward, avg_episode_length


    def model_training_step(self):
        # A function that optimizes the model self.model using the optimizer 
        # self.optimizer using the experience  stored in self.expert_buffer.
        # Number of optimization step should be self.cfg.num_training_steps.

        # Set the model to training.
        self.model.train()
        # For num training steps, sample data from the training data.
        avg_loss = 0.
        for _ in range(self.cfg.num_training_steps):
            # TODO write the training code.
            # Hint: use the self.transforms to make sure the image observation is of the right size.
            self.optimizer.zero_grad()
            sampled_obs, sampled_actions = self.expert_buffer.sample(self.cfg.batch_size)
            sampled_obs=torch.from_numpy(sampled_obs).float().to(self.device)
            sampled_actions=torch.from_numpy(sampled_actions).float().to(self.device)
            t_obs=self.transforms(sampled_obs)
            predicted_actions=self.model(t_obs)
            loss_=self.loss_function(predicted_actions, sampled_actions)
            loss_.backward()
            self.optimizer.step()
            avg_loss+=loss_.item()
            
        avg_loss /= self.cfg.num_training_steps
        return avg_loss


    def run(self):
        train_loss, eval_reward, episode_length = 1., 0, 0
        iterable = tqdm.trange(self.cfg.total_training_episodes)
        
        global_reward=float('-inf')
        for ep_num in iterable:
            iterable.set_description('Collecting exp')
            # Set the DAGGER model to evaluation
            self.model.eval()
            ep_train_reward = 0.
            ep_length = 0.

            ## Added by sumanyu
            obs = self.train_env.reset()
            exp_act=self.train_env.get_expert_action()
            self.expert_buffer.insert(obs, exp_act)
            ## Adding the expert action for first state

            # TODO write the training loop.
            # 1. Roll out your current model on the environment.
            # 2. On each step, after calling either env.reset() or env.step(), call 
            #    env.get_expert_action() to get the expert action for the current 
            #    state of the environment.
            # 3. Store that observation alongside the expert action in the buffer.
            # 4. When you are training, use the stored obs and expert action.
            
            # Hints:
            # 1. You will need to convert your obs to a torch tensor before passing it
            #    into the model.
            # 2. You will need to convert your action predicted by the model to a numpy
            #    array before passing it to the environment.
            # 3. Make sure the actions from your model are always in the (-1, 1) range.
            # 4. Both the environment observation and the expert action needs to be a
            #    numpy array before being added to the environment.
            # 5. Use the self.transforms to make sure the image observation is of the right size.
            
            # TODO training loop here.
            
            ### Sumanyu's Code
            done=False
            while not done:
                obs = torch.from_numpy(obs).float().to(self.device).unsqueeze(0)
                t_obs=self.transforms(obs)
                predicted_action=self.model(t_obs)
                if predicted_action.device=="cuda":
                    predicted_action=predicted_action.squeeze().detach().cpu().numpy()
                else:
                    predicted_action=predicted_action.squeeze().detach().numpy()
                # print(predicted_action)
                obs, reward, done, _ = self.train_env.step(predicted_action)
                exp_act = self.train_env.get_expert_action()
                self.expert_buffer.insert(obs, exp_act)
                ep_train_reward+=reward
                ep_length+=1
                    
            train_reward = ep_train_reward
            train_episode_length = ep_length

            if (ep_num + 1) % self.cfg.train_every == 0:
                # Reinitialize model every time we are training
                iterable.set_description('Training model')
                self.model, self.optimizer = initialize_model_and_optim(self.cfg)

                # TODO train the model and set train_loss to the appropriate value.
                train_loss=self.model_training_step()
                # Hint: in the DAgger algorithm, when do we initialize a new model?

            if (ep_num + 1) % self.cfg.eval_every == 0:
                # Evaluation loop
                iterable.set_description('Evaluating model')
                eval_reward, episode_length = self.eval()
                if eval_reward>global_reward:
                    global_reward=eval_reward
                    torch.save(self.model.state_dict(), os.path.join(self.cfg.save_dir, "policy.pt"))

            iterable.set_postfix({
                'Train loss': train_loss,
                'Train reward': train_reward,
                'Eval reward': eval_reward
            })


@hydra.main(config_path='.', config_name='train')
def main(cfg):
    # In hydra, whatever is in the train.yaml file is passed on here
    # as the cfg object. To access any of the parameters in the file,
    # access them like cfg.param, for example the learning rate would
    # be cfg.lr
    workspace = Workspace(cfg)
    workspace.run()


if __name__ == '__main__':
    main()
